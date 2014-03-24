#include <iostream>
#include <boost/asio.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-exception.h"

#include "vtrc-client-base/vtrc-client.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "protocol/vtrc-service.pb.h"

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
    vtrc::shared_ptr<client::vtrc_client> cl(
                          client::vtrc_client::create((tp.get_io_service( ))));

    ///cl.connect( "127.0.0.1", "44667" );
    cl->async_connect( "127.0.0.1", "44667", on_connect );

    vtrc::this_thread::sleep_for( vtrc::chrono::milliseconds(2000) );

    vtrc::shared_ptr<google::protobuf::RpcChannel> ch(cl->get_channel( ));
    vtrc_service::test_rpc::Stub s( ch.get( ) );

    vtrc_rpc_lowlevel::message_info mi;

    size_t last = 0;
    for( int i=0; i<200000000; ++i ) {
        try {
            s.test( NULL, &mi, &mi, NULL );
            last = mi.message_type( );
            //std::cout << "response: " << last << "\n";
            //cl.reset( );
        } catch( const vtrc::common::exception &ex ) {
//            std::cout << "call error: "
//                      << " code (" << ex.code( ) << ")"
//                      << " category (" << ex.category( ) << ")"
//                      << " what: " << ex.what( )
//                      << " (" << ex.additional( ) << ")"
//                      << "\n";
            std::cout << " last  " << last << "\n";
        } catch( const std::exception &ex ) {
            std::cout << "call error: " << ex.what( ) << "\n";
        }
    }

    tp.stop( );
    tp.join_all( );

    return 0;

}
