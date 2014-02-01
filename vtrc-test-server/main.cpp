#include <iostream>
#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <stdlib.h>

#include "vtrc-server/vtrc-application-iface.h"
#include "vtrc-server/vtrc-endpoint-iface.h"
#include "vtrc-server/vtrc-connection-iface.h"
#include "vtrc-server/vtrc-endpoint-tcp.h"

#include "vtrc-common/vtrc-thread-poll.h"
#include "vtrc-common/vtrc-sizepack-policy.h"

#include "vtrc-common/vtrc-hasher-iface.h"
#include "vtrc-common/vtrc-hasher-impls.h"

#include "vtrc-common/vtrc-data-queue.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-auth.pb.h"

namespace ba = boost::asio;

class main_app: public vtrc::server::application_iface
{
    ba::io_service ios_;

    typedef
    std::map <
        vtrc::server::connection_iface *,
        boost::shared_ptr<vtrc::server::connection_iface>
    > connection_map_type;

    connection_map_type connections_;
    boost::shared_mutex connections_lock_;

    typedef boost::unique_lock<boost::shared_mutex> unique_lock;
    typedef boost::shared_lock<boost::shared_mutex> shared_lock;

public:

    main_app( )
    {}

    ba::io_service &get_io_service( )
    {
        return ios_;
    }

private:

    void on_endpoint_started( vtrc::server::endpoint_iface *ep )
    {
        std::cout << "Start endpoint: " << ep->string( ) << "\n";
    }

    void on_endpoint_stopped( vtrc::server::endpoint_iface *ep )
    {
        std::cout << "Stop endpoint: " << ep->string( ) << "\n";
    }

    void on_endpoint_exception( vtrc::server::endpoint_iface *ep )
    {
        try {
            throw;
        } catch( const std::exception &ex ) {
            std::cout << "Endpoint exception '" << ep->string( )
                      << "': " << ex.what( ) << "\n";
        } catch( ... ) {
            throw;
        }
    }

    void on_new_connection_accepted(
                    vtrc::server::connection_iface* connection )
    {
        unique_lock l( connections_lock_ );
        connections_.insert( std::make_pair(connection,
              boost::shared_ptr<vtrc::server::connection_iface>(connection) ) );
        std::cout << "connection accepted\n";
    }

    void on_new_connection_ready(
                            vtrc::server::connection_iface* connection )
    {
        std::cout << "connection ready\n";
    }

    void on_connection_die( vtrc::server::connection_iface* connection )
    {
        unique_lock l( connections_lock_ );
        connections_.erase( connection );
        std::cout << "connection die\n";
    }

};


int main( )
{

    vtrc::common::data_queue::queue_base_sptr pack
            (vtrc::common::data_queue::varint::create_serializer(1000));

    vtrc::common::data_queue::queue_base_sptr collector
            (vtrc::common::data_queue::varint::create_parser(1000));

    vtrc_auth::init_hello t;

    t.set_hello_message( "123" );

    std::string ser(t.SerializeAsString());

    pack->append( ser.c_str( ), ser.size( ) );
    pack->process( );

    pack->append( ser.c_str( ), ser.size( ) );
    pack->process( );

    pack->append( ser.c_str( ), ser.size( ) );
    pack->process( );

    std::string p;
    for( size_t i=0; i<pack->messages( ).size( ); ++i )
        p += pack->messages( )[i];

    std::cout << "packed: '" << p << "'\n";

    collector->append( p.c_str( ), p.size( ) - 2 );
    collector->process( );

    std::cout << "unpacked: '" << collector->messages( ).size( ) << "'\n";
    for( size_t i=0; i<collector->messages( ).size( ); ++i )
    {
        vtrc_auth::init_hello t;
        t.ParseFromString(collector->messages( )[i]);
        std::cout << t.DebugString( ) << "\n";
    }

    return 0;

    main_app app;
    vtrc::common::thread_poll poll(app.get_io_service( ), 4);

    boost::shared_ptr<vtrc::server::endpoint_iface> tcp_ep
            (vtrc::server::endpoints::tcp::create(app, "0.0.0.0", 44667));

    tcp_ep->start( );

    poll.join_all( );

    return 0;
}
