#include <iostream>

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-tcp.h"
#include "vtrc-server/vtrc-channels.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-stub-wrapper.h"

#include "protocol/hello-events.pb.h"   /// hello protocol
#include "google/protobuf/descriptor.h" /// for descriptor( )->full_name( )
#include "boost/lexical_cast.hpp"

using namespace vtrc;

namespace {

class  hello_service_impl: public howto::hello_events_service {

    typedef howto::hello_events_service super_type;

    common::connection_iface *cl_;

    void generate_events(::google::protobuf::RpcController* /*controller*/,
                 const ::howto::request_message*            /*request*/,
                 ::howto::response_message*                 /*response*/,
                 ::google::protobuf::Closure* done)
    {
        common::closure_holder ch( done ); /// instead of done->Run( );
        typedef howto::hello_events_Stub stub_type;

        using   server::channels::unicast::create_callback_channel;
        using   server::channels::unicast::create_event_channel;


        { // do event. send and dont wait response
            common::rpc_channel *ec =
                            create_event_channel( cl_->shared_from_this( ) );

            common::stub_wrapper<stub_type> event( ec );
            event.call( &stub_type::hello_event );
        }

        { // do callback. wait response from client.
            common::rpc_channel *cc =
                            create_callback_channel( cl_->shared_from_this( ) );

            common::stub_wrapper<stub_type> callback( cc );
            howto::event_res response;

            callback.call_response( &stub_type::hello_callback, &response );

            std::cout << "Client string: "
                      << response.hello_from_client( )
                      << "\n"
                      ;
        }
    }

public:

    hello_service_impl( common::connection_iface *cl )
        :cl_(cl)
    { }

    static std::string const &service_name(  )
    {
        return super_type::descriptor( )->full_name( );
    }
};

class hello_application: public server::application {

    typedef common::rpc_service_wrapper     wrapper_type;
    typedef vtrc::shared_ptr<wrapper_type>  wrapper_sptr;

public:

    hello_application( common::pool_pair &pp )
        :server::application(pp)
    { }

    wrapper_sptr get_service_by_name( common::connection_iface* connection,
                                      const std::string &service_name )
    {
        if( service_name == hello_service_impl::service_name( ) ) {

             hello_service_impl *new_impl = new hello_service_impl(connection);

             return vtrc::make_shared<wrapper_type>( new_impl );

        }
        return wrapper_sptr( );
    }

};

} // namespace

int main( int argc, const char **argv )
{

    const char *address = "127.0.0.1";
    unsigned short port = 56560;

    if( argc > 2 ) {
        address = argv[1];
        port = boost::lexical_cast<unsigned short>( argv[2] );
    } else if( argc > 1 ) {
        port = boost::lexical_cast<unsigned short>( argv[1] );
    }

    common::pool_pair pp( 0, 1 );
    hello_application app( pp );

    try {

        vtrc::shared_ptr<server::listener>
                tcp( server::listeners::tcp::create( app, address, port ) );

        tcp->start( );

        pp.get_io_pool( ).attach( );

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    pp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}

