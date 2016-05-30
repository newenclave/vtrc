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

//#include "vtrc-server/vtrc-protocol-layer.h"

using namespace vtrc;

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

        std::cout << "make call for '" << cl_->name( ) << "'\n";

        oss << "Hello " << request->name( )
            << " from hello_service_impl::send_hello!\n"
            << "Your transport name is '"
            << cl_->name( ) << "'.\nHave a nice day.";

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

class connection: public common::connection_iface {

    connection( )
    { }

    vtrc::unique_ptr<common::protocol_iface> protocol_;

    common::protocol_iface &get_protocol( )
    {
        return *protocol_;
    }

    const common::protocol_iface &get_protocol( ) const
    {
        return *protocol_;
    }

    vtrc::weak_ptr<server::listeners::custom> lst_;

public:

    static
    vtrc::shared_ptr<connection> create(
            server::listeners::custom::shared_type &l )
    {
        connection *n = new connection;
        n->lst_ = l;
        vtrc::shared_ptr<connection> ns(n);

//        /// have to create and init protocol!
//        n->protocol_.reset( server::protocol::create( app, ns ) );
//        n->protocol_->init( );

        return ns;
    }

    void init( )
    {

    }

    void set_protocol( common::protocol_iface* protocol )
    {
        protocol_.reset( protocol );
    }

    std::string name( ) const
    {
        return "customtransport://1";
    }

    const std::string &id( )  const
    {
        static const std::string id( "" );
        return id;
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

    bool active( ) const
    {
        return true;
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

    common::native_handle_type native_handle( ) const
    {
        common::native_handle_type res;
        res.value.ptr_ = NULL;
        return res;
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

        server::listeners::custom::shared_type l =
                server::listeners::custom::create( app, "custom" );

        l->assign_lowlevel_protocol_factory( &common::lowlevel::dummy::create );
        common::connection_iface_sptr c = l->accept<connection>( l );

        //common::connection_iface_sptr c(connection::create( app ));

        if( FILE *f = fopen( filename, "rb" ) ) {
            char buf[256];
            while( size_t t = fread( buf, 1, 256, f ) ) {
                c->get_protocol( ).process_data( buf, t );
            }
            fclose( f );
            tp.get_io_service( ).post( vtrc::bind( stop, vtrc::ref( tp ) ) );
        }
        tp.attach( );
        tp.stop( );
        tp.join_all( );

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}

