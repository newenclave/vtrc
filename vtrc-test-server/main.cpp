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
        connection->write( "hello!", 6 );
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

    google::protobuf::Service *create_service(
            const std::string &name, vtrc::server::connection_iface* connection)
    {
        return NULL;
    }

};


int main( )
{

    main_app app;
    vtrc::common::thread_poll poll(app.get_io_service( ), 4);

    boost::shared_ptr<vtrc::server::endpoint_iface> tcp_ep
            (vtrc::server::endpoints::tcp::create(app, "0.0.0.0", 44667));

    tcp_ep->start( );

    poll.join_all( );

    return 0;
}
