#include <iostream>
#include <boost/asio.hpp>

#include "vtrc-common/vtrc-thread-poll.h"
#include "vtrc-client-base/vtrc-client.h"

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

    tp.join_all( );

    return 0;

}
