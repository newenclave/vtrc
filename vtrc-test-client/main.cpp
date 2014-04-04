#include <iostream>
#include <boost/asio.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

#include "vtrc-common/vtrc-pool-pair.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-call-context.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-protocol-layer.h"

#include "vtrc-client-base/vtrc-client.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "protocol/vtrc-service.pb.h"
#include "protocol/vtrc-errors.pb.h"

#include <boost/random.hpp>
#include <boost/random/random_device.hpp>

#include "vtrc-thread.h"
#include "vtrc-chrono.h"

using namespace vtrc;

void on_connect( const boost::system::error_code &err )
{
    std::cout << "connected "
              << err.value( ) << " "
              << err.message( ) << "\n";
}

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

class ping_impl: public vtrc_service::internal {

    client::vtrc_client *c_;

public:

    ping_impl( client::vtrc_client *c )
        :c_( c )
    {}

    void ping(::google::protobuf::RpcController* controller,
                         const ::vtrc_service::ping_req* request,
                         ::vtrc_service::pong_res* response,
                         ::google::protobuf::Closure* done)
    {
        common::closure_holder chold(done);
        std::cout << "ping event rcvd "
                  << c_->connection( )
                     ->get_call_context( )
                     ->get_lowlevel_message( )->id( )
                  << " " << vtrc::this_thread::get_id( ) << " "
                  //<< vtrc::chrono::high_resolution_clock::now( )
                  << "\n";

        return;

        const vtrc::common::call_context *cc =
                    vtrc::common::call_context::get( c_->connection( ) );

        vtrc::shared_ptr<google::protobuf::RpcChannel>
                            ch(c_->create_channel( false, true ));

        vtrc_service::test_message      mi;
        vtrc_service::test_rpc::Stub s( ch.get( ) );

        s.test2( NULL, &mi, &mi, NULL );

        //if( done ) done->Run( );
    }
};

class test_ev: public vtrc_service::test_events
{
    vtrc::common::connection_iface *c_;
public:
    test_ev( vtrc::common::connection_iface *c )
        :c_( c )
    {}

    void test(::google::protobuf::RpcController* controller,
                         const ::vtrc_service::test_message* request,
                         ::vtrc_service::test_message* response,
                         ::google::protobuf::Closure* done)
    {
        common::closure_holder ch(done);
        std::cout << "test event rcvd "
                  << c_->get_call_context( )->get_lowlevel_message( )->id( )
                  << " " << vtrc::this_thread::get_id( ) << " "
                  << "\n";
    }

};

void run_client( vtrc::shared_ptr<client::vtrc_client> cl, bool wait)
{
    vtrc::shared_ptr<google::protobuf::RpcChannel>
            ch(cl->create_channel( wait, false ));
    vtrc_service::test_rpc::Stub s( ch.get( ) );

    vtrc_service::test_message mi;
    vtrc_service::test_message mir;

    size_t last = 0;

    std::string ts(1024 * 800, 0);

    std::cout << "this thread: "
              << vtrc::this_thread::get_id( ) << " "
              << "\n";

    for( int i=0; i<29999999999; ++i ) {
        try {

            if( wait )
                vtrc::this_thread::sleep_for( vtrc::chrono::milliseconds(1) );
            work_time wt;
            //mi.set_b( ts );
            s.test( NULL, &mi, &mir, NULL );
            last = mir.id( );
            std::cout << "response: " << last << "\n";
            //cl.reset( );
        } catch( const vtrc::common::exception &ex ) {
            std::cout << "call error: "
                      << " code (" << ex.code( ) << ")"
                      << " category (" << ex.category( ) << ")"
                      << " what: " << ex.what( )
                      << " (" << ex.additional( ) << ")"
                      << "\n";
            //if( i % 100 == 0 )
            std::cout << i << "\n";
            if( ex.category( ) == vtrc_errors::CATEGORY_SYSTEM ) break;
            if( ex.code( ) == vtrc_errors::ERR_COMM ) break;
        } catch( const std::exception &ex ) {
            //std::cout << "call error: " << ex.what( ) << "\n";
        }
    }
}

int main( )
{

    common::pool_pair pp(2, 2);
    vtrc::shared_ptr<client::vtrc_client> cl(client::vtrc_client::create(pp));

//    cl->connect( "/tmp/test" );
    //cl->connect( "192.168.56.101", "44667" );
    //cl->connect( "\\\\.\\pipe\\test_pipe");
    cl->connect( "127.0.0.1", "44667" );
    //cl->connect( "::1", "44668" );
    ///cl->async_connect( "127.0.0.1", "44667", on_connect );

    cl->advise_handler( vtrc::shared_ptr<test_ev>(new test_ev(cl->connection( ).get( ))) );
    cl->advise_handler( vtrc::shared_ptr<ping_impl>(new ping_impl(cl.get( ))) );

    vtrc::this_thread::sleep_for( vtrc::chrono::milliseconds(2000) );

    //vtrc::thread( run_client, cl, true ).detach( );
    //vtrc::thread( run_client, cl, false ).detach( );
    vtrc::thread( run_client, cl, false ).join( );

    pp.stop_all( );
    pp.join_all( );

    return 0;

}
