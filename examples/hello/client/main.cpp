#include <iostream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-stub-wrapper.h"

#include "protocol/hello.pb.h"

#include "boost/lexical_cast.hpp"

using namespace vtrc;

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

int main( int argc, const char **argv )
{
    common::thread_pool tp( 1 );

    const char *address = "127.0.0.1";
    unsigned short port = 56560;

    if( argc > 2 ) {
        address = argv[1];
        port = boost::lexical_cast<unsigned short>( argv[2] );
    } else if( argc > 1 ) {
        port = boost::lexical_cast<unsigned short>( argv[1] );
    }

    client::vtrc_client_sptr cl(
                client::vtrc_client::create( tp.get_io_service( ) ) );

    cl->on_connect_connect( on_connect );
    cl->on_ready_connect( on_ready );
    cl->on_disconnect_connect( on_disconnect );

    try {

        std::cout <<  "Connecting..." << std::endl;

        cl->connect( address, port );
        std::cout << "Ok" << std::endl;

        vtrc::unique_ptr<common::rpc_channel> channel(cl->create_channel( ));

        common::stub_wrapper<howto::hello_service_Stub> hello(channel.get( ));

        howto::request_message  req;
        howto::response_message res;

        req.set_name( "%USERNAME%" );

        typedef howto::hello_service_Stub stub_type;

        hello.call( &stub_type::send_hello, &req, &res );

        std::cout <<  res.hello( ) << std::endl;

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    tp.stop( );
    tp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}
