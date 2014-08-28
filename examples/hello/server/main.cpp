#include <iostream>

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-tcp.h"

#include "vtrc-common/vtrc-connection-iface.h"

#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"

#include "protocol/hello.pb.h" /// hello protocol

#include "boost/lexical_cast.hpp"

#include "google/protobuf/descriptor.h" /// for descriptor( )->full_name( )


using namespace vtrc;

namespace {

class  hello_service_impl: public howto::hello_service {

    common::connection_iface *cl_;

    void send_hello(::google::protobuf::RpcController*  controller,
                    const ::howto::request_message*     request,
                    ::howto::response_message*          response,
                    ::google::protobuf::Closure*        done) /*override*/
    {
        std::ostringstream oss;

        oss << "Hello " << request->name( )
            << " from hello_service_impl::send_hello!\n"
            << "Your transport name is '"
            << cl_->name( ) << "'.\nHave a nice day.";

        response->set_hello( oss.str( ) );
        done->Run( );
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

class hello_application: public server::application {

    typedef common::rpc_service_wrapper     wrapper_type;
    typedef vtrc::shared_ptr<wrapper_type>  wrapper_sptr;

public:

    hello_application( common::thread_pool &tp )
        :server::application(tp.get_io_service( ))
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

}

int main( int argc, const char **argv )
{
    common::thread_pool tp;
    hello_application app( tp );

    const char *address = "127.0.0.1";
    unsigned short port = 56560;

    if( argc > 2 ) {
        address = argv[1];
        port = boost::lexical_cast<unsigned short>( argv[2] );
    } else if( argc > 1 ) {
        port = boost::lexical_cast<unsigned short>( argv[1] );
    }

    try {

        vtrc::shared_ptr<server::listener>
                tcp( server::listeners::tcp::create( app, address, port ) );

        tcp->start( );

        tp.attach( );

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }


    tp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}

