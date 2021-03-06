#include <iostream>

#include "vtrc/server/application.h"
#include "vtrc/server/listener/tcp.h"
#include "vtrc/server/channels.h"

#include "vtrc/common/connection-iface.h"
#include "vtrc/common/closure-holder.h"
#include "vtrc/common/pool-pair.h"
#include "vtrc/common/stub-wrapper.h"

#include "protocol/hello-events.pb.h"   /// hello protocol
#include "google/protobuf/descriptor.h" /// for descriptor( )->full_name( )
#include "boost/lexical_cast.hpp"

#include "vtrc-system.h"
#include "../protocol/http-header.hpp"

#include <set>

using namespace vtrc;

namespace {

namespace vc = vtrc::common;

std::set<std::string> run_params;

class http_lowlevel_server: vc::lowlevel::protocol_layer_iface {

    typedef vc::lowlevel::protocol_layer_iface::message_type message_type;

    typedef http::header_parser             parser_type;
    typedef vtrc::shared_ptr<parser_type>   parser_sptr;
    typedef std::deque<parser_sptr>         requests_container;

    vc::protocol_accessor   *pa_;
    requests_container       container_;
    parser_sptr              current_header_;

public:

    http_lowlevel_server( )
        :pa_(NULL)
    { }

    static
    vc::lowlevel::protocol_layer_iface *create( )
    {
        return new http_lowlevel_server;
    }

    void reset_current( )
    {
        current_header_ = vtrc::make_shared<parser_type>( );
    }

    void init( vc::protocol_accessor *pa,
               vc::system_closure_type ready_cb )
    {
        pa_ = pa;
        reset_current( );
        ready_cb( VTRC_SYSTEM::error_code( ) );
        pa->ready( true );
    }

    std::string serialize_lowlevel( const lowlevel_unit &mess )
    {
        return pack_message( mess );
    }

    std::string serialize_message( const message_type *mess )
    {
        return mess->SerializeAsString( );
    }

    void parse_message( const std::string &data, message_type *mess )
    {
        mess->ParseFromString( data );
    }

    std::string pack_message( const rpc::lowlevel_unit &mess )
    {
        std::string d = http::lowlevel2http( mess );
        if( run_params.find( "-d" ) != run_params.end( ) ) {
            std::cout << "========\n" << d << "\n";
        }
        return d;
    }

    void process_data( const char *data, size_t length )
    {
        bool ok = false;

        //std::cout << std::string( data, length );

        while( length ) {
            size_t r = current_header_->push( data, data + length );
            length -= r;
            data   += r;
            if( current_header_->ready( ) ) {
                container_.push_back( current_header_ );
                ok = true;
                reset_current( );
            }
        }
        if( ok ) {
            pa_->message_ready( );
        }
    }

    size_t queue_size( ) const
    {
        return container_.size( );
    }

    bool pop_proto_message( vtrc::rpc::lowlevel_unit &result )
    {
        bool res = http::http2lowlevel( *container_.front( ), result );
        container_.pop_front( );
        return res;
    }

    void close( ) { }

};


class  hello_service_impl: public howto::hello_events_service {

    typedef howto::hello_events_service super_type;

    common::connection_iface *cl_;

    void generate_events(::google::protobuf::RpcController* /*controller*/,
                 const ::howto::request_message*            /*request*/,
                 ::howto::response_message*                 /*response*/,
                 ::google::protobuf::Closure* done)
    {
        common::closure_holder ch( done ); /// instead of done->Run( );
        typedef howto::hello_events_Stub stub_type;

        using   server::channels::unicast::create_callback_channel;
        using   server::channels::unicast::create_event_channel;


        { // do event. send and dont wait response
            common::rpc_channel *ec =
                            create_event_channel( cl_->shared_from_this( ) );

            common::stub_wrapper<stub_type> event( ec );
            event.call( &stub_type::hello_event );
        }

        { // do callback. wait response from client.
            common::rpc_channel *cc =
                            create_callback_channel( cl_->shared_from_this( ) );

            common::stub_wrapper<stub_type> callback( cc );
            howto::event_res response;

            callback.call_response( &stub_type::hello_callback, &response );

            std::cout << "Client string: "
                      << response.hello_from_client( )
                      << "\n"
                      ;
        }
    }

    void say_hello(::google::protobuf::RpcController* controller,
                         const ::howto::request_message* request,
                         ::howto::response_message* response,
                         ::google::protobuf::Closure* done)
    {
        common::closure_holder ch( done ); /// instead of done->Run( );
        response->set_hello( "Hello!" );
    }

public:

    hello_service_impl( common::connection_iface *cl )
        :cl_(cl)
    { }

    static std::string const &service_name(  )
    {
        return super_type::descriptor( )->full_name( );
    }
};

class hello_application: public server::application {

    typedef common::rpc_service_wrapper     wrapper_type;
    typedef vtrc::shared_ptr<wrapper_type>  wrapper_sptr;

public:

    hello_application( common::pool_pair &pp )
        :server::application(pp)
    { }

    wrapper_sptr get_service_by_name( common::connection_iface* connection,
                                      const std::string &service_name )
    {
        if( service_name == hello_service_impl::service_name( ) ) {

             hello_service_impl *new_impl = new hello_service_impl(connection);

             return vtrc::make_shared<wrapper_type>( new_impl );

        }
        return wrapper_sptr( );
    }

};

} // namespace

int main( int argc, const char **argv )
{

    const char *address = "127.0.0.1";
    unsigned short port = 56560;

    for( int i=0; i<argc; ++i ) {
        run_params.insert( argv[i] );
    }

    if( argc > 2 ) {
        address = argv[1];
        port = boost::lexical_cast<unsigned short>( argv[2] );
    } else if( argc > 1 ) {
        port = boost::lexical_cast<unsigned short>( argv[1] );
    }

    common::pool_pair pp( 0, 1 );
    hello_application app( pp );

    try {

        rpc::session_options opts = vtrc::common::defaults::session_options( );
        //opts.set_read_buffer_size( 1 ); // test

        vtrc::shared_ptr<server::listener>
                tcp( server::listeners::tcp::create( app, opts,
                                                     address, port ) );

        tcp->assign_lowlevel_protocol_factory( &http_lowlevel_server::create );

        tcp->start( );

        pp.get_io_pool( ).attach( );

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    pp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}

