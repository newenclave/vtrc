#include <iostream>

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-tcp.h"
#include "vtrc-server/vtrc-listener-ssl.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-delayed-call.h"

#include "protocol/hello.pb.h"          /// hello protocol
#include "google/protobuf/descriptor.h" /// for descriptor( )->full_name( )
#include "boost/lexical_cast.hpp"

#include "openssl/ssl.h"
#include "openssl/err.h"

#include "../protocol/ssl-wrapper.h"

using namespace vtrc;

namespace {

const std::string CERTF = "../server.crt";
const std::string KEYF  = "../server.key";

class my_ssl_wrapper: public ssl_wrapper_server {

public:
    my_ssl_wrapper( )
        :ssl_wrapper_server(TLSv1_2_server_method( ))
    { }

private:

    void init_context( )
    {
        if ( SSL_CTX_use_certificate_file( get_context( ), CERTF.c_str( ),
                                           SSL_FILETYPE_PEM) <= 0) {
            howto::ssl_exception::raise( "SSL_CTX_use_certificate_file" );
        }

        if ( SSL_CTX_use_PrivateKey_file( get_context( ), KEYF.c_str( ),
                                          SSL_FILETYPE_PEM) <= 0) {
            howto::ssl_exception::raise( "SSL_CTX_use_PrivateKey_file" );
        }

        if ( !SSL_CTX_check_private_key( get_context( ) ) ) {
            std::runtime_error( "SSL_CTX_check_private_key failed!" );
        }
    }
};

class  hello_ssl_service_impl: public howto::hello_ssl_service {

    common::connection_iface *cl_;
    my_ssl_wrapper            ssl_;

    void send_block(::google::protobuf::RpcController*  /*controller*/,
                    const ::howto::request_message*     request,
                    ::howto::response_message*          response,
                    ::google::protobuf::Closure*        done) /*override*/
    {
        common::closure_holder ch( done ); /// instead of done->Run( );

        if( !ssl_.init_finished( ) ) {
            std::string hsres = ssl_.do_handshake( request->block( ).c_str( ),
                                                   request->block( ).size( ) );
            response->set_block( hsres );
        } else {
            std::string decres = ssl_.decrypt(request->block( ));
            std::cout <<  "Request: "  << decres << "\n";
            response->set_block( ssl_.encrypt(
                                     std::string( decres.rbegin( ),
                                                  decres.rend( ) ) ) );
        }

    }

public:

    hello_ssl_service_impl( common::connection_iface *cl )
        :cl_(cl)
    {
        ssl_.init( );
    }

    ~hello_ssl_service_impl( )
    {

    }

    static std::string const &service_name(  )
    {
        return howto::hello_ssl_service::descriptor( )->full_name( );
    }

};

class hello_application: public server::application {

    typedef common::rpc_service_wrapper     wrapper_type;
    typedef vtrc::shared_ptr<wrapper_type>  wrapper_sptr;

public:

    hello_application( common::thread_pool &tp )
        :server::application(tp.get_io_service( ))
    { }

    wrapper_sptr get_service_by_name( common::connection_iface* connection,
                                      const std::string &service_name )
    {
        if( service_name == hello_ssl_service_impl::service_name( ) ) {

             hello_ssl_service_impl *new_impl
                     = new hello_ssl_service_impl(connection);

             return vtrc::make_shared<wrapper_type>( new_impl );

        }
        return wrapper_sptr( );
    }

};

} // namespace

int main( int argc, const char **argv )
{

    for( int i=0; i<100; ++i ) {
    howto::bio_wrapper b(BIO_f_base64( ));
    howto::bio_wrapper bs(BIO_s_mem( ));

    b.append( bs );
    b.write( "test12345\n", 10 );

    b.flush( );

    std::cout << bs.read( 100 ) << "\n";
    }
    return 1;

    const char *address = "127.0.0.1";
    unsigned short port = 56560;

    if( argc > 2 ) {
        address = argv[1];
        port = boost::lexical_cast<unsigned short>( argv[2] );
    } else if( argc > 1 ) {
        port = boost::lexical_cast<unsigned short>( argv[1] );
    }

    SSL_load_error_strings( );
    SSLeay_add_ssl_algorithms( );

    common::thread_pool tp;
    hello_application app( tp );

    try {

        vtrc::common::delayed_call dc(tp.get_io_service( ));

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

