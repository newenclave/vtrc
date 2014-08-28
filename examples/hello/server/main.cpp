#include <iostream>

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-tcp.h"
#include "vtrc-server/vtrc-listener-local.h"

#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"

#include "protocol/hello.pb.h" /// hello protocol

#include "boost/lexical_cast.hpp"

using namespace vtrc;

namespace {

class  hello_service_impl: public howto::hello_service {

    void send_hello(::google::protobuf::RpcController*  controller,
                    const ::howto::request_message*     request,
                    ::howto::response_message*          response,
                    ::google::protobuf::Closure*        done) /*override*/
    {
        std::ostringstream oss;

        oss << "Hello " << request->name( )
            << " from hello_service_impl::send_hello!";

        response->set_hello( oss.str( ) );
        done->Run( );
    }

public:
    static std::string const &service_name(  )
    {
        return howto::hello_service::descriptor( )->full_name( );
    }

};

class hello_application: public server::application {

    typedef vtrc::shared_ptr<common::rpc_service_wrapper> wrapper_sptr;

public:

    hello_application( vtrc::common::pool_pair &pp )
        :vtrc::server::application(pp)
    { }

    wrapper_sptr get_service_by_name( common::connection_iface* connection,
                                      const std::string &service_name )
    {
        if( service_name == hello_service_impl::service_name( ) ) {

             hello_service_impl *new_impl = new  hello_service_impl;

             return vtrc::make_shared<common::rpc_service_wrapper>( new_impl );

        }
        return wrapper_sptr( );
    }

};

}

int main( int argc, const char **argv )
{
    common::pool_pair pp( 1, 1 );

    hello_application app( pp );

    const char *address = "127.0.0.1";
    unsigned short port = 56560;

    if( argc > 2 ) {
        address = argv[1];
        port = boost::lexical_cast<unsigned short>( argv[2] );
    } else if( argc > 1 ) {
        port = boost::lexical_cast<unsigned short>( argv[1] );
    }

    vtrc::shared_ptr<server::listener>
            tcp( server::listeners::tcp::create( app, address, port ) );

    tcp->start( );

    pp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}

