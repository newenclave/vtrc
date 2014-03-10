#include <iostream>
#include <boost/asio.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-client-base/vtrc-client.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include <boost/random.hpp>
#include <boost/random/random_device.hpp>

#include "vtrc-thread.h"
#include "vtrc-chrono.h"

void on_connect( const boost::system::error_code &err )
{
    std::cout << "connected "
              << err.value( ) << " "
              << err.message( ) << "\n";
}

using namespace vtrc;

int main( )
{
    common::thread_pool tp(4);
    client::vtrc_client cl( tp.get_io_service( ) );

    ///cl.connect( "127.0.0.1", "44667" );
    cl.async_connect( "127.0.0.1", "44667", on_connect );

    vtrc::this_thread::sleep_for( vtrc::chrono::milliseconds(2000) );

    vtrc_rpc_lowlevel::test_rpc::Stub s( cl.get_channel( ).get( ) );

    vtrc_rpc_lowlevel::message_info mi;

    for( int i=0; i<10000000; ++i ) {
        s.test( NULL, &mi, &mi, NULL );
        std::cout << "response: " << mi.message_type( ) << "\n";
    }

    tp.join_all( );

    return 0;

}
