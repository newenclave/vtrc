#include <iostream>

#include "vtrc/server/application.h"
#include "vtrc/server/listener/tcp.h"
#include "vtrc/server/listener/ssl.h"

#include "vtrc/common/connection-iface.h"
#include "vtrc/common/closure-holder.h"
#include "vtrc/common/thread-pool.h"
#include "vtrc/common/closure.h"
#include "vtrc/common/delayed-call.h"
#include "vtrc/common/call-context.h"

#include "protocol/hello.pb.h"          /// hello protocol
#include "google/protobuf/descriptor.h" /// for descriptor( )->full_name( )
#include "boost/lexical_cast.hpp"

using namespace vtrc;

namespace {

class  hello_service_impl: public howto::hello_service {

    common::connection_iface *cl_;

    void send_hello(::google::protobuf::RpcController*  /*controller*/,
                    const ::howto::request_message*     request,
                    ::howto::response_message*          response,
                    ::google::protobuf::Closure*        done) /*override*/
    {
        common::closure_holder ch( done ); /// instead of done->Run( );
        std::ostringstream oss;

        oss << "Hello " << request->name( )
            << " from hello_service_impl::send_hello!\n"
            << "Your transport name is '"
            << cl_->name( ) << "'.\nHave a nice day.";

//        std::cout << "Context: "
//                  << std::hex
//                  << common::call_context::get( ) << "\n";

        response->set_hello( oss.str( ) );
        /// done->Run( ); /// ch will call it
    }

public:

    hello_service_impl( common::connection_iface *cl )
        :cl_(cl)
    { }

    static std::string const &service_name(  )
    {
        return howto::hello_service::descriptor( )->full_name( );
    }

};

vtrc::shared_ptr<google::protobuf::Service> make_service(
                                           common::connection_iface* connection,
                                           const std::string &service_name )
{
    if( service_name == hello_service_impl::service_name( ) ) {
        std::cout << "Create service " << service_name
                  << " for " << connection->name( )
                  << "\n";
        return vtrc::make_shared<hello_service_impl>( connection );
    }
    return vtrc::shared_ptr<google::protobuf::Service>( );
}

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

    common::thread_pool tp;
    server::application app( tp.get_io_service( ) );

    app.assign_service_factory( &make_service );

    try {

        vtrc::common::delayed_call dc(tp.get_io_service( ));
//        dc.call_from_now( [&tp]( ... ){ tp.stop( ); },
//                          boost::posix_time::seconds(20) );

        vtrc::shared_ptr<server::listener>
                tcp( server::listeners::tcp::create( app, address, port ) );

        tcp->start( );

        tp.attach( );

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    tp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}

