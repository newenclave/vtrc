#include <iostream>

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-stub-wrapper.h"

#include "vtrc-thread.h"

#include "protocol/hello-events.pb.h"

#include "boost/lexical_cast.hpp"

#include "google/protobuf/descriptor.h"

using namespace vtrc;
namespace gpb = google::protobuf;

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

vtrc::shared_ptr<gpb::Service> m_events( const std::string &name )
{
    if( name == howto::hello_events::descriptor( )->full_name( ) ) {
        return vtrc::make_shared<hello_event_impl>( );
    }
    return vtrc::shared_ptr<gpb::Service>( );
}

int main( int argc, const char **argv )
{
    common::pool_pair pp( 1, 1 );

    const char *address = "0.0.0.0";
    unsigned short port = 56560;

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

        cl->assign_service_factory( m_events );

        //cl->assign_rpc_handler( vtrc::make_shared<hello_event_impl>( ) );

        std::cout <<  "Connecting..." << std::endl;

        cl->connect( address, port );
        std::cout << "Ok" << std::endl;

        vtrc::unique_ptr<common::rpc_channel> channel(cl->create_channel( ));

        typedef howto::hello_events_service_Stub stub_type;

        common::stub_wrapper<stub_type> hello(channel.get( ));

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
