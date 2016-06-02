#include <iostream>
#include <stdio.h>

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-tcp.h"
#include "vtrc-server/vtrc-listener-ssl.h"
#include "vtrc-server/vtrc-listener-custom.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-delayed-call.h"
#include "vtrc-common/vtrc-connection-list.h"

#include "vtrc-bind.h"
#include "vtrc-ref.h"

#include "protocol/hello.pb.h"          /// hello protocol
#include "google/protobuf/descriptor.h" /// for descriptor( )->full_name( )
#include "boost/lexical_cast.hpp"

#include "vtrc-common/vtrc-protocol-iface.h"

#include "vtrc-rpc-lowlevel.pb.h"

//#include "vtrc-server/vtrc-protocol-layer.h"

using namespace vtrc;

static uint64_t count = 0;

namespace {

class  hello_service_impl: public howto::hello_service {

    common::connection_iface *cl_;

    void send_hello(::google::protobuf::RpcController*  /*controller*/,
                    const ::howto::request_message*     request,
                    ::howto::response_message*          response,
                    ::google::protobuf::Closure*        done) /*override*/
    {
        common::closure_holder ch( done ); /// instead of done->Run( );
        std::ostringstream oss;

        //        std::cout << "make call for '" << cl_->name( ) << "'\n";

        //        oss << "Hello " << request->name( )
        //            << " from hello_service_impl::send_hello!\n"
        //            << "Your transport name is '"
        //            << cl_->name( ) << "'.\nHave a nice day.";

        count++;
        response->set_hello( oss.str( ) );
        /// done->Run( ); /// ch will call it
    }

public:

    hello_service_impl( common::connection_iface *cl )
        :cl_(cl)
    { }

    static std::string const &service_name(  )
    {
        return howto::hello_service::descriptor( )->full_name( );
    }

};

vtrc::shared_ptr<google::protobuf::Service> make_service(
        common::connection_iface* connection,
        const std::string &service_name )
{
    if( service_name == hello_service_impl::service_name( ) ) {
        std::cout << "Create service " << service_name
                  << " for " << connection->name( )
                  << std::endl;
        return vtrc::make_shared<hello_service_impl>( connection );
    }
    return vtrc::shared_ptr<google::protobuf::Service>( );
}

class connection: public common::connection_empty {

    connection( )
    { }

    vtrc::weak_ptr<server::listeners::custom> lst_;

public:

    static
    vtrc::shared_ptr<connection> create(
            server::listeners::custom::shared_type &l )
    {
        connection *n = new connection;
        n->lst_ = l;
        vtrc::shared_ptr<connection> ns(n);
        return ns;
    }

    void close( )
    {
        if( !lst_.expired( ) ) {
            server::listeners::custom::shared_type l = lst_.lock( );
            if( l ) {
                l->stop_client( this );
            }
        }
        std::cout << "closed!\n";
        /// does nothing
    }

    void write( const char *data, size_t length )
    {
        //std::cout << std::string( data, length );
    }

    void write( const char *data, size_t length,
                const common::system_closure_type &success,
                bool success_on_send )
    {
        //std::cout << std::string( data, length );
        success( VTRC_SYSTEM::error_code( ) );
    }

};


} // namespace

void stop( common::thread_pool &tp )
{
    tp.stop( );
}

int main( int argc, const char **argv )
{

    common::thread_pool tp(0);
    server::application app( tp.get_io_service( ) );

    const char *filename = argc > 1 ? argv[1] : "message.txt";

    app.assign_service_factory( &make_service );

    try {

        rpc::session_options opts = common::defaults::session_options( );

        opts.set_max_active_calls( 1000000 );

        server::listeners::custom::shared_type l =
                server::listeners::custom::create( app, opts, "custom" );

        l->assign_lowlevel_protocol_factory( &common::lowlevel::dummy::create );
        common::connection_iface_sptr c = l->accept<connection>( l );

        //common::connection_iface_sptr c(connection::create( app ));

        std::vector<char> data;

        using namespace vtrc::chrono;

        high_resolution_clock::time_point start;

        if( FILE *f = fopen( filename, "rb" ) ) {
            char buf[1024];
            while( size_t t = fread( buf, 1, 1024, f ) ) {
                data.insert( data.end( ), buf, buf + t );
            }
            fclose( f );
            start = high_resolution_clock::now( );
            c->get_protocol( ).process_data( &data[0], data.size( ) );
            tp.get_io_service( ).post( vtrc::bind( stop, vtrc::ref( tp ) ) );
        }
        tp.attach( );
        std::cout << "microsec: "
                  << duration_cast<microseconds>(high_resolution_clock::now( )
                                                 - start).count( )
                  << "\n";
        tp.stop( );
        std::cout << count << "\n";
        tp.join_all( );

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}

