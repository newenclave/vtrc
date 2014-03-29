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
#include "vtrc-server/vtrc-endpoint-unix-local.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-pool-pair.h"
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
#include "vtrc-thread.h"

#include "vtrc-server/vtrc-channels.h"

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

void test_send( common::connection_iface *connection,
                vtrc::server::application &app )
{
    common::connection_iface_sptr s(connection->shared_from_this());
    vtrc::shared_ptr<google::protobuf::RpcChannel> ev(
                vtrc::server
                ::channels::unicast
                ::create_event_channel( s, false ));

    const vtrc_rpc_lowlevel::lowlevel_unit *pllu =
            common::call_context::get( s )->get_lowlevel_message( );

//    vtrc_rpc_lowlevel::lowlevel_unit llu;
//    s->get_protocol( ).send_message( llu );

    vtrc_service::internal::Stub ping( ev.get( ));
    vtrc_service::ping_req preq;
    vtrc_service::pong_res pres;

//    for( int i=0; i<100; ++i )
//        ping.ping( NULL, &preq, &pres, NULL );

    try {
        //for( ;; )
        {
            std::cout << "ping 2 " << vtrc::this_thread::get_id( ) << "\n";
            ping.ping( NULL, &preq, &pres, NULL );
        }
    } catch( std::exception const &ex ) {
        std::cout << "png error " << ex.what( ) << "\n";
    }

}

class teat_impl: public vtrc_service::test_rpc {

    common::connection_iface *c_;
    vtrc::server::application &app_;
    unsigned id_;

public:

    teat_impl( common::connection_iface *c, vtrc::server::application &app )
        :c_(c)
        ,app_(app)
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
//        test_send(c_, app_);

        if( done ) done->Run( );
    }

    virtual void test2(::google::protobuf::RpcController* controller,
                         const ::vtrc_rpc_lowlevel::message_info* request,
                         ::vtrc_rpc_lowlevel::message_info* response,
                         ::google::protobuf::Closure* done)
    {
        const vtrc::common::call_context *cc =
                    vtrc::common::call_context::get( c_ );

        std::cout << "test 2 " << vtrc::this_thread::get_id( ) << "\n\n";

        std::cout << "stack: ";
        while (cc) {
            std::cout << cc->get_lowlevel_message( )->call( ).method_id( )
                      << " <- ";
            cc = cc->parent( );
        }
        std::cout << "\n";

        if( done ) done->Run( );
    }
};

class main_app: public vtrc::server::application
{

public:

    main_app( common::pool_pair &pp )
        :application(pp)
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

    vtrc::common::rpc_service_wrapper_sptr get_service_by_name(
                                    vtrc::common::connection_iface *connection,
                                    const std::string &service_name)
    {
        if( service_name == teat_impl::descriptor( )->full_name( ) ) {
            return common::rpc_service_wrapper_sptr(
                        new common::rpc_service_wrapper(
                                new teat_impl(connection, *this) ) );
        }

        return common::rpc_service_wrapper_sptr( );
    }

};

int main( ) try {

    common::pool_pair pp(1, 1);
    main_app app(pp);

    vtrc::shared_ptr<vtrc::server::endpoint_iface> tcp_ep
            (vtrc::server::endpoints::tcp::create(app, "0.0.0.0", 44667));

#ifndef _WIN32

    vtrc::shared_ptr<vtrc::server::endpoint_iface> tcp_ul
            (vtrc::server::endpoints::unix_local::create(app, "/tmp/test"));
    tcp_ul->start( );

#endif

    tcp_ep->start( );

    boost::this_thread::sleep_for( vtrc::chrono::milliseconds(12000) );

    std::cout << "Stoppped. Wait ... \n";

#ifndef _WIN32
    tcp_ul->stop( );
#endif
    tcp_ep->stop( );

    std::cout << "Stoppped. Wait ... \n";

    pp.stop_all( );
    pp.join_all( );

    google::protobuf::ShutdownProtobufLibrary( );

    return 0;

} catch( const std::exception &ex ) {

    std::cout << "general error: " << ex.what( ) << "\n";
    return 0;
}
