#include <iostream>

#include "vtrc/client/client.h"
#include "vtrc/common/thread-pool.h"
#include "vtrc/common/stub-wrapper.h"

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

    const char *address = "\\\\.\\pipe\\hello_windows_impersonation_pipe";

    if( argc > 2 ) {
        address = argv[1];
    }


    try {

        client::vtrc_client_sptr cl =
                         client::vtrc_client::create( tp.get_io_service( ) );

        cl->on_connect_connect( on_connect );
        cl->on_ready_connect( on_ready );
        cl->on_disconnect_connect( on_disconnect );

        std::cout <<  "Connecting..." << std::endl;

        cl->connect( address );
        std::cout << "Ok" << std::endl;

        vtrc::unique_ptr<common::rpc_channel> channel(cl->create_channel( ));

        typedef howto::hello_service_Stub stub_type;

        common::stub_wrapper<stub_type> hello(channel.get( ));

        howto::request_message  req;
        howto::response_message res;

        req.set_name( "%USERNAME%" );

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
