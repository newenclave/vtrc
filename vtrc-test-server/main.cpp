#include <iostream>
#include <stdlib.h>

#include <vector>
#include <queue>

#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/chrono.hpp>

#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <boost/thread/condition_variable.hpp>

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-endpoint-iface.h"
#include "vtrc-server/vtrc-endpoint-tcp.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-sizepack-policy.h"

#include "vtrc-common/vtrc-hasher-iface.h"

#include "vtrc-common/vtrc-data-queue.h"
#include "vtrc-common/vtrc-conditional-queues.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-auth.pb.h"


namespace ba = boost::asio;

using namespace vtrc;

class main_app: public vtrc::server::application
{

public:

    main_app( )
    { }

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
                    vtrc::common::connection_iface* connection )
    {
        std::cout << "connection accepted\n";
    }

    void on_new_connection_ready(
                            vtrc::common::connection_iface* connection )
    {
        std::cout << "connection ready\n";
    }

    void on_connection_die( vtrc::common::connection_iface* connection )
    {
        std::cout << "connection die\n";
    }

    google::protobuf::Service *create_service(
            const std::string &name, vtrc::common::connection_iface* connection)
    {
        return NULL;
    }

    vtrc::common::rpc_service_wrapper_sptr get_service_by_name(
                                    vtrc::common::connection_iface *connection,
                                    const std::string &service_name)
    {
        return common::rpc_service_wrapper_sptr( );
    }


};

int main( ) try {

    main_app app;
    vtrc::common::thread_pool poll(app.get_io_service( ), 4);

    boost::shared_ptr<vtrc::server::endpoint_iface> tcp_ep
            (vtrc::server::endpoints::tcp::create(app, "0.0.0.0", 44667));

    tcp_ep->start( );

    poll.join_all( );

    return 0;

} catch( ... ) {

    return 0;
}
