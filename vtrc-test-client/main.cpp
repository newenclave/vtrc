#include <iostream>
#include <boost/asio.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

#include "vtrc-common/vtrc-thread-poll.h"
#include "vtrc-client-base/vtrc-client.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

void on_connect( const boost::system::error_code &err )
{
    std::cout << "connected "
              << err.value( ) << " "
              << err.message( ) << "\n";
}

using namespace vtrc;


int main( )
{

    common::thread_poll tp(4);
    client::vtrc_client cl( tp.get_io_service( ) );

    cl.connect( "127.0.0.1", "44667" );
    ///cl.async_connect( "127.0.0.1", "44667", on_connect );

    vtrc_rpc_lowlevel::test_rpc::Stub s( cl.get_channel( ).get( ) );

    vtrc_rpc_lowlevel::message_info mi;


    s.test( NULL, &mi, &mi, NULL );

    tp.join_all( );

    return 0;

}
