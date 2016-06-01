#include <iostream>

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-stub-wrapper.h"

#include "protocol/hello.pb.h"

#include "boost/lexical_cast.hpp"

#include "vtrc-system.h"

#include <fcntl.h>

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
    void on_error( const rpc::errors::container &, const char *mess )
    {
        std::cout << mess << "...";
    }


    class connection: public common::connection_empty {

        connection( )
        { }

    public:

        static
        vtrc::shared_ptr<connection> create( )
        {
            connection *n = new connection;
            vtrc::shared_ptr<connection> ns(n);

            return ns;
        }

        void close( )
        {
            std::cout << "closed!\n";
            /// does nothing
        }

        void write( const char *data, size_t length )
        {
            std::cerr << std::string( data, length );
        }

        void write( const char *data, size_t length,
                    const common::system_closure_type &success,
                    bool /*success_on_send*/ )
        {
            std::cerr << std::string( data, length );
            success( VTRC_SYSTEM::error_code( ) );
        }

    };
}

int main( int argc, const char **argv )
{
    common::thread_pool tp( 1 );

    try {

        client::vtrc_client_sptr cl =
                         client::vtrc_client::create( tp.get_io_service( ) );

        cl->on_connect_connect( on_connect );
        cl->on_ready_connect( on_ready );
        cl->on_disconnect_connect( on_disconnect );
        cl->on_init_error_connect( on_error );

        cl->assign_lowlevel_protocol_factory( &common::lowlevel::dummy::create );

        std::cout <<  "Connecting..." << std::endl;

        vtrc::shared_ptr<connection> c = cl->assign<connection>( );

        std::cout << "Ok" << std::endl;

        vtrc::unique_ptr<common::rpc_channel> channel(cl->create_channel( ));

        channel->set_flag( common::rpc_channel::DISABLE_WAIT );

        typedef howto::hello_service_Stub stub_type;

        common::stub_wrapper<stub_type> hello(channel.get( ));

        howto::request_message  req;
        howto::response_message res;

        req.set_name( "%USERNAME%" );

        for( int i=0; i<1000000; ++i ) {
            hello.call( &stub_type::send_hello, &req, &res );
        }

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
