#include <iostream>
#include <stdlib.h>

#include <vector>
#include <queue>

#include <google/protobuf/descriptor.h>

#include <boost/asio.hpp>
#include <boost/chrono.hpp>

#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <boost/thread/condition_variable.hpp>

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-endpoint-iface.h"
#include "vtrc-server/vtrc-endpoint-tcp.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-sizepack-policy.h"
#include "vtrc-common/vtrc-exception.h"

#include "vtrc-common/vtrc-hash-iface.h"

#include "vtrc-common/vtrc-data-queue.h"
#include "vtrc-common/vtrc-condition-queues.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-protocol-layer.h"
#include "vtrc-common/vtrc-call-context.h"
#include "vtrc-common/vtrc-connection-iface.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-auth.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "protocol/vtrc-service.pb.h"

#include "vtrc-memory.h"
#include "vtrc-chrono.h"

#include "vtrc-server/vtrc-unicast-channels.h"

namespace ba = boost::asio;

using namespace vtrc;

struct work_time {
    typedef boost::chrono::high_resolution_clock::time_point time_point;
    time_point start_;
    work_time( )
        :start_(boost::chrono::high_resolution_clock::now( ))
    {}
    ~work_time( )
    {
        time_point stop(boost::chrono::high_resolution_clock::now( ));
        std::cout << "call time: " << stop - start_ << "\n";
    }
};

void test_send( common::connection_iface *connection )
{
    common::connection_iface_sptr s(connection->shared_from_this());
    vtrc::shared_ptr<google::protobuf::RpcChannel> ev(
                vtrc::server
                ::channels::unicast
                ::create_event_channel(s, false));

    const vtrc_rpc_lowlevel::lowlevel_unit *pllu =
            s->get_protocol( ).
            get_call_context( )->get_lowlevel_message( );

    vtrc_rpc_lowlevel::lowlevel_unit llu(*pllu);
    //s->get_protocol( ).send_message( llu );

    vtrc_service::internal::Stub ping( ev.get( ));
    vtrc_service::ping_req preq;
    vtrc_service::pong_res pres;

//    for( int i=0; i<100; ++i )
//        ping.ping( NULL, &preq, &pres, NULL );

    try {
        //for( ;; )
        {
            //ping.ping( NULL, &preq, &pres, NULL );
//            std::cout << llu->id( ) << " sent "
//                         << vtrc::chrono::high_resolution_clock::now( )
//                         << "\n";
            //boost::this_thread::sleep_for( vtrc::chrono::milliseconds(10) );
        }
    } catch( std::exception const &ex ) {
        std::cout << "png error " << ex.what( ) << "\n";
    }

}

class teat_impl: public vtrc_service::test_rpc {

    common::connection_iface *connection_;
    unsigned id_;

public:

    teat_impl(common::connection_iface *c )
        :connection_(c)
        ,id_(0)
    { }

    void test(::google::protobuf::RpcController* controller,
              const ::vtrc_rpc_lowlevel::message_info* request,
              ::vtrc_rpc_lowlevel::message_info* response,
              ::google::protobuf::Closure* done)
    {

        response->set_message_type( id_++ );
        if( (id_ % 100) == 0 )
            throw std::runtime_error( "oops 10 =)" );

//        connection_->get_io_service( ).dispatch(
//                    vtrc::bind(test_send, connection_));
//        boost::thread(test_send, connection_).detach( );
        test_send(connection_);

        if( done ) done->Run( );
    }
};

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
        if( service_name == teat_impl::descriptor( )->full_name( ) ) {
            return common::rpc_service_wrapper_sptr(
                        new common::rpc_service_wrapper(
                                new teat_impl(connection) ) );
        }

        return common::rpc_service_wrapper_sptr( );
    }

};

int main( ) try {

    main_app app;
    vtrc::common::thread_pool poll(app.get_io_service( ), 4);

    vtrc::shared_ptr<vtrc::server::endpoint_iface> tcp_ep
            (vtrc::server::endpoints::tcp::create(app, "0.0.0.0", 44667));

    tcp_ep->start( );

    boost::this_thread::sleep_for( vtrc::chrono::milliseconds(12000) );

    std::cout << "Stoppped. Wait ... \n";

    tcp_ep->stop( );

    poll.stop( );
    poll.join_all( );

    return 0;

} catch( ... ) {

    return 0;
}
