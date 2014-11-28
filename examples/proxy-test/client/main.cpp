#include <iostream>

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-common/protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-thread.h"
#include "vtrc-memory.h"
#include "vtrc-chrono.h"

#include "vtrc-common/vtrc-call-context.h"

#include "protocol/client-calls.pb.h"
#include "protocol/proxy-calls.pb.h"

#include "boost/lexical_cast.hpp"

using namespace vtrc;

class hello_service: public proxy::hello {

    client::vtrc_client *cl_;

public:
    hello_service( client::vtrc_client_sptr cl )
        :cl_(cl.get( ))
    { }
    void send_hello(::google::protobuf::RpcController* controller,
                 const ::proxy::hello_req* request,
                 ::proxy::hello_res* response,
                 ::google::protobuf::Closure* done)
    {
        common::closure_holder holder( done );

        const common::call_context *cc = cl_->get_call_context( );

        std::cout << "Request rcved! ";

        if( cc ) {
            std::cout << "channel data: " << cc->channel_data( );
        } else {
            std::cout << "Context is empty :(";
        }

        std::cout << std::endl;

        response->set_pong( request->ping( ) + ": pong" );
    }
};

class hello_channel: public common::rpc_channel {

    typedef proxy::transmitter::Stub                proxy_stub_type;
    typedef common::stub_wrapper<proxy_stub_type>   proxy_type;


    vtrc::shared_ptr<common::rpc_channel> parent_;
    std::string                           tgt_;
    proxy_type                            proxy_;

public:

    hello_channel( client::vtrc_client_sptr c, std::string const &tgt )
        :common::rpc_channel( rpc::message_info::MESSAGE_CLIENT_CALL, 0 )
        ,parent_(c->create_channel( ))
        ,tgt_(tgt)
        ,proxy_(parent_)
    { }

    bool alive( ) const { return parent_->alive( ); }

    void send_message( lowlevel_unit_type &llu,
                const google::protobuf::MethodDescriptor* method,
                      google::protobuf::RpcController* /*controller*/,
                const google::protobuf::Message* request,
                      google::protobuf::Message* response,
                      google::protobuf::Closure* /*done*/ )
    {
        vtrc::shared_ptr<rpc::lowlevel_unit> req_llu;
        req_llu = parent_->make_lowlevel( method, request, response );
        proxy::proxy_message req;
        proxy::proxy_message res;
        req.set_client_id( tgt_ );
        req.set_data( req_llu->SerializeAsString( ) );
        proxy_.call( &proxy_stub_type::send_to, &req, &res );
        if( response ) {
            rpc::lowlevel_unit res_llu;
            res_llu.ParseFromString( res.data( ) );
            response->ParseFromString( res_llu.response( ) );
        }
    }
};

namespace {
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
}

std::string make_call( client::vtrc_client_sptr cl, const char *target )
{
    typedef proxy::hello::Stub              stub_type;
    typedef common::stub_wrapper<stub_type> hello_calls;

    hello_channel hc( cl, target );

    hello_calls call_hello(&hc);
    proxy::hello_req req;
    proxy::hello_res res;

    req.set_ping( "Hello there!" );

    call_hello.call( &stub_type::send_hello, &req, &res );

    return res.pong( );
}

int main( int argc, const char **argv )
{
    common::pool_pair pp( 1, 1 );

    const char *address = "127.0.0.1";
    const char *target  = NULL;
    unsigned short port = 56560;

    address = argv[1];
    port = boost::lexical_cast<unsigned short>( argv[2] );
    if( argc > 3 ) {
        target = argv[3];
    }

    try {

        client::vtrc_client_sptr cl = client::vtrc_client::create( pp );

        cl->on_connect_connect( on_connect );
        cl->on_ready_connect( on_ready );
        cl->on_disconnect_connect( on_disconnect );

        cl->assign_rpc_handler( vtrc::make_shared<hello_service>( cl ) );

        std::cout <<  "Connecting..." << std::endl;

        cl->connect( address, port );

        std::cout << "Ok" << std::endl;

        if( target ) {

            std::cout << make_call( cl, target ) << "\n";

        } else {
            typedef proxy::transmitter::Stub                proxy_stub_type;
            typedef common::stub_wrapper<proxy_stub_type>   proxy_type;

            proxy_type st(cl->create_channel( ), true);
            st.call( &proxy_stub_type::reg_for_proxy );

            vtrc::this_thread::sleep_for( vtrc::chrono::hours( 1 ) );
        }

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    pp.stop_all( );
    pp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}
