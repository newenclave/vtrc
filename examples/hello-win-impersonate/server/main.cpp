#include <iostream>

#include "vtrc/server/application.h"
#include "vtrc/server/listener/tcp.h"
#include "vtrc/server/listener/local.h"

#include "vtrc/common/connection-iface.h"
#include "vtrc/common/closure-holder.h"
#include "vtrc/common/thread-pool.h"
#include "vtrc/common/closure.h"

#include "protocol/hello.pb.h"          /// hello protocol
#include "google/protobuf/descriptor.h" /// for descriptor( )->full_name( )
#include "boost/lexical_cast.hpp"

#include "vtrc/common/closure.h"
#include "vtrc/common/connection-iface.h"
#include "vtrc/common/exception.h"

#include <Windows.h>

using namespace vtrc;

namespace {

class impersonator {
    bool impersonated_;
public:
    impersonator( common::connection_iface *cl,
        const google::protobuf::MethodDescriptor *meth )
        :impersonated_( false )
    {
        BOOL res = ImpersonateNamedPipeClient(
            cl->native_handle( ).value.win_handle );
        if( !res ) {
            std::cerr << "ImpersonateNamedPipeClient failed for "
                << meth->full_name( )
                << ": " << GetLastError( ) << "\n"
                ;
        } else {
            impersonated_ = true;
            std::cout << "ImpersonateNamedPipeClient Ok."
                << "\n"
                ;
        }
    }

    ~impersonator( )
    {
        if( impersonated_ ) {
            BOOL res = RevertToSelf( );
            if( !res ) {
                std::cerr << "RevertToSelf failed "
                    << GetLastError( ) << "\n";
                ;
            } else {
                std::cout << "RevertToSelf Ok."
                    << "\n"
                    ;
            }
        }
    }
};


class  hello_service_impl: public howto::hello_service {

    common::connection_iface *cl_;

    void send_hello(::google::protobuf::RpcController*  /*controller*/,
                    const ::howto::request_message*     request,
                    ::howto::response_message*          response,
                    ::google::protobuf::Closure*        done) /*override*/
    {
        common::closure_holder ch( done ); /// instead of done->Run( );
        std::ostringstream oss;

        char    block[1024];
        DWORD   size = 1024;
        BOOL res = GetUserName( block, &size );
        std::string un = res ? block : "<failed to get user name>";

        oss << "Hello " << request->name( )
            << " from hello_service_impl::send_hello!\n"
            << "Your transport name is '"
            << cl_->name( ) 
            << "'.\nUser name " << un << "\n"
            << "Have a nice day.";


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

class hello_application: public server::application {

    typedef common::rpc_service_wrapper                   wrapper_type;
    typedef vtrc::shared_ptr<wrapper_type>                wrapper_sptr;
    typedef common::rpc_service_wrapper::service_type     service_type;

public:

    hello_application( common::thread_pool &tp )
        :server::application(tp.get_io_service( ))
    { }

    class service_wrapper: public wrapper_type {
        common::connection_iface* c_;
    public:
        service_wrapper( common::connection_iface* c, service_type *svc )
            :wrapper_type(svc)
            ,c_(c)
        { }
        void call_method( const method_type *method,
            google::protobuf::RpcController* controller,
            const google::protobuf::Message* request,
            google::protobuf::Message* response,
            google::protobuf::Closure* done )
        {
            impersonator imp( c_, method );
            call_default( method, controller, 
                          request, response, done );
        }
    };

    wrapper_sptr get_service_by_name( common::connection_iface* c,
                                      const std::string &service_name )
    {
        if( service_name == hello_service_impl::service_name( ) ) {

             hello_service_impl *new_impl = new hello_service_impl(c);
             return vtrc::make_shared<service_wrapper>( c, new_impl );

        }
        return wrapper_sptr( );
    }

};

} // namespace

int main( int argc, const char **argv )
{

    const char *address = "\\\\.\\pipe\\hello_windows_impersonation_pipe";

    if( argc > 2 ) {
        address = argv[1];
    }

    common::thread_pool tp;
    hello_application app( tp );

    try {

        vtrc::shared_ptr<server::listener>
             pipe( server::listeners::local::create( app, address ) );

        pipe->start( );

        tp.attach( );

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    tp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}

