
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
#include "vtrc-common/vtrc-closure-holder.h"
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
                ::create_event_channel( s, true));

    if(common::call_context::get( s ))
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
//            std::cout << "ping 2 " << vtrc::this_thread::get_id( ) << "\n";
            ping.ping( NULL, &preq, &pres, NULL );
        }
    } catch( std::exception const &ex ) {
//        std::cout << "png error " << ex.what( ) << "\n";
    }

}

void call_delayed_test( ::google::protobuf::RpcController* controller,
                        const ::vtrc_rpc_lowlevel::message_info* request,
                        ::vtrc_rpc_lowlevel::message_info* response,
                        ::google::protobuf::Closure* done,
                        unsigned id,
                        common::connection_iface *c_,
                        vtrc::server::application &app_)
{

    common::closure_holder ch(done);
    response->set_message_type( id );

    test_send(c_, app_);
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

        common::closure_holder ch(done);

//        app_.get_rpc_service( ).post( boost::bind( call_delayed_test,
//                                      controller, request, response, done,
//                                                   ++id_,
//                                                   c_, vtrc::ref(app_)) );

        response->set_message_type( id_++ );
        if( (id_ % 100) == 0 )
            throw std::runtime_error( "oops 10 =)" );

//        connection_->get_io_service( ).dispatch(
//                    vtrc::bind(test_send, connection_));
//        boost::thread(test_send, connection_).detach( );
//        test_send(c_, app_);
    }

    virtual void test2(::google::protobuf::RpcController* controller,
                         const ::vtrc_rpc_lowlevel::message_info* request,
                         ::vtrc_rpc_lowlevel::message_info* response,
                         ::google::protobuf::Closure* done)
    {
        common::closure_holder ch(done);

        const vtrc::common::call_context *cc =
                    vtrc::common::call_context::get( c_ );

//        std::cout << "test 2 " << vtrc::this_thread::get_id( ) << "\n\n";

        std::cout << std::setw(8) << id_ << " stack: ";
        while (cc) {
            std::cout << cc->get_lowlevel_message( )->call( ).method_id( )
                      << " <- ";
            cc = cc->next( );
        }
        std::cout << "\n";
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

    common::pool_pair pp(2, 4);
    main_app app(pp);

    vtrc::shared_ptr<vtrc::server::endpoint_iface> tcp4_ep
            (vtrc::server::endpoints::tcp::create(app, "0.0.0.0", 44667));

    vtrc::shared_ptr<vtrc::server::endpoint_iface> tcp6_ep
            (vtrc::server::endpoints::tcp::create(app, "::", 44668));

#ifndef _WIN32

    std::string file_name("/tmp/test");

    ::unlink( file_name.c_str( ) );
    vtrc::shared_ptr<vtrc::server::endpoint_iface> tcp_ul
            (vtrc::server::endpoints::unix_local::create(app, file_name));
    ::chmod(file_name.c_str( ), 0xFFFFFF );

    tcp_ul->start( );

#endif

    tcp4_ep->start( );
    tcp6_ep->start( );

    boost::this_thread::sleep_for( vtrc::chrono::milliseconds(120009999) );

    std::cout << "Stoppped. Wait ... \n";

#ifndef _WIN32
    tcp_ul->stop( );
#endif

    tcp4_ep->stop( );
    tcp6_ep->stop( );

    app.stop_all_clients( );

    std::cout << "Stoppped. Wait ... \n";

    pp.stop_all( );
    pp.join_all( );

    google::protobuf::ShutdownProtobufLibrary( );

    return 0;

} catch( const std::exception &ex ) {

    std::cout << "general error: " << ex.what( ) << "\n";
    return 0;
}
