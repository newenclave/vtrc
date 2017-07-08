#include <iostream>

#include "vtrc/client/client.h"
#include "vtrc/common/pool-pair.h"
#include "vtrc/common/stub-wrapper.h"
#include "vtrc/common/lowlevel-protocol-iface.h"

#include "vtrc-thread.h"
#include "vtrc-system.h"

#include "protocol/hello-events.pb.h"

#include "../protocol/http-header.hpp"

#include "boost/lexical_cast.hpp"

#include "google/protobuf/descriptor.h"

#include <set>

using namespace vtrc;
namespace gpb = google::protobuf;

namespace {

    std::set<std::string> run_params;


    void on_connect( )
    {
        std::cout << "connect...";
    }
    void on_ready( )
    {
        std::cout << "ready...";
    }
    void on_disconnect( )
    {
        std::cout << "disconnect...";
    }

    namespace vc = vtrc::common;

    class http_lowlevel_client: vc::lowlevel::protocol_layer_iface {

        typedef vc::lowlevel::protocol_layer_iface::message_type message_type;
        typedef http::header_parser             parser_type;
        typedef vtrc::shared_ptr<parser_type>   parser_sptr;
        typedef std::deque<parser_sptr>         requests_container;

        vc::protocol_accessor   *pa_;
        requests_container       container_;
        parser_sptr              current_header_;
    public:

        http_lowlevel_client( )
            :pa_(NULL)
        { }

        static
        vc::lowlevel::protocol_layer_iface *create( )
        {
            return new http_lowlevel_client;
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

}

class hello_event_impl: public howto::hello_events {

    void hello_event(::google::protobuf::RpcController* /*controller*/,
                 const ::howto::event_req*              /*request*/,
                 ::howto::event_res*                    /*response*/,
                 ::google::protobuf::Closure* done)
    {
        common::closure_holder ch( done );
        std::cout << "Event from server; Current thread is: 0x"
                  << std::hex
                  << vtrc::this_thread::get_id( )
                  << std::dec
                  << "\n"
                     ;
    }

    void hello_callback(::google::protobuf::RpcController* /*controller*/,
                 const ::howto::event_req*                 /*request*/,
                 ::howto::event_res* response,
                 ::google::protobuf::Closure* done)
    {
        common::closure_holder ch( done );
        std::cout << "Callback from server; Current thread is: 0x"
                  << std::hex
                  << vtrc::this_thread::get_id( )
                  << std::dec
                  << "\n"
                     ;

        std::ostringstream oss;
        oss << "Hello there! my thread id is 0x"
            << std::hex
            << vtrc::this_thread::get_id( )
               ;
        response->set_hello_from_client( oss.str( ) );
    }
};

vtrc::common::rpc_service_wrapper_sptr make_events( const std::string &name )
{
    typedef vtrc::common::rpc_service_wrapper wrapper;
    typedef vtrc::common::rpc_service_wrapper_sptr wrapper_sptr;

    if( name == howto::hello_events::descriptor( )->full_name( ) ) {
        return vtrc::make_shared<wrapper>(new hello_event_impl);
    }
    return wrapper_sptr( );
}

int main( int argc, const char **argv )
{
    common::pool_pair pp( 1, 1 );

    const char *address = "0.0.0.0";
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

    try {

        client::vtrc_client_sptr cl = client::vtrc_client::create( pp );

        cl->on_connect_connect( on_connect );
        cl->on_ready_connect( on_ready );
        cl->on_disconnect_connect( on_disconnect );

        cl->assign_service_factory( make_events );
        cl->assign_lowlevel_protocol_factory( &http_lowlevel_client::create );

        //cl->assign_rpc_handler( vtrc::make_shared<hello_event_impl>( ) );

        std::cout <<  "Connecting..." << std::endl;

        cl->connect( address, port );
        std::cout << "Ok" << std::endl;

        typedef howto::hello_events_service_Stub stub_type;

        common::stub_wrapper<stub_type> hello(cl->create_channel( ), true);

        std::cout << "main( ) thread id is: 0x"
                  << std::hex
                  << vtrc::this_thread::get_id( )
                  << std::dec
                  << "\n";

        std::cout << "Make call 'generate_events'...\n";
        hello.call( &stub_type::generate_events );
        std::cout << "'generate_events' OK\n";

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    pp.stop_all( );
    pp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}
