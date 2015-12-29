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

using namespace vtrc;

namespace {

const std::string CERTF = "../server.crt";
const std::string KEYF  = "../server.key";

void ssl_throw( const char *add )
{
    std::string err(add);
    err += ": ";
    size_t final = err.size( );
    err.resize( final + 1024 );
    ERR_error_string_n(ERR_get_error( ), &err[final], err.size( ) - final );
    std::cerr << "Ssl exceptrion. " << err << "\n";
    throw std::runtime_error( err );
}

class  hello_ssl_service_impl: public howto::hello_ssl_service {

    common::connection_iface *cl_;
    SSL_CTX* ctx_;
    SSL*     ssl_;
    BIO*     in_;
    BIO*     out_;

    void send_block(::google::protobuf::RpcController*  /*controller*/,
                    const ::howto::request_message*     request,
                    ::howto::response_message*          response,
                    ::google::protobuf::Closure*        done) /*override*/
    {
        common::closure_holder ch( done ); /// instead of done->Run( );
        std::ostringstream oss;

        size_t block_size = request->block( ).size( );

        std::cout << "Block: " << block_size << "\n";

        BIO_write( in_, request->block( ).c_str( ), request->block( ).size( ) );

        if( !SSL_is_init_finished( ssl_ ) ) {
            int n = SSL_do_handshake( ssl_ );
            if( n <= 0 ) {
                int err = SSL_get_error( ssl_, n );
                std::cout << "Err: " << err << "\n";
                if( err == SSL_ERROR_WANT_READ ) {
                    std::cout << "More encrypted data required for handshake\n";
                } else if( err == SSL_ERROR_WANT_WRITE ) {
                    std::cout << "Writting of data required for handshake\n";
                } else if( err == SSL_ERROR_NONE ) {
                    std::cout << "No error but not accepted connection\n";
                } else {
                  ssl_throw( "SSL_accept" );
                }
                response->set_error( err );
            }
            char *data;
            size_t length = BIO_get_mem_data( out_, &data );
            response->set_block( data, length );
            return;
        }
    }

public:

    hello_ssl_service_impl( common::connection_iface *cl )
        :cl_(cl)
        ,ctx_(NULL)
        ,ssl_(NULL)
        ,in_(NULL)
        ,out_(NULL)
    {
        const SSL_METHOD *meth = SSLv23_server_method( );
        ctx_ = SSL_CTX_new (meth);
        if ( !ctx_ ) {
            ssl_throw( "SSL_CTX_new" );
        }

        if ( SSL_CTX_use_certificate_file( ctx_, CERTF.c_str( ),
                                           SSL_FILETYPE_PEM) <= 0) {
            ssl_throw( "SSL_CTX_use_certificate_file" );
        }

        if ( SSL_CTX_use_PrivateKey_file( ctx_, KEYF.c_str( ),
                                          SSL_FILETYPE_PEM) <= 0) {
            ssl_throw( "SSL_CTX_use_PrivateKey_file" );
        }

        if ( !SSL_CTX_check_private_key( ctx_ ) ) {
            std::runtime_error( "SSL_CTX_check_private_key failed!" );
        }
        ssl_ = SSL_new( ctx_ );
        if( !ssl_ ) {
            ssl_throw( "SSL_new" );
        }

        in_  = BIO_new( BIO_s_mem( ) );
        out_ = BIO_new( BIO_s_mem( ) );
        if( !in_ || !out_ ) {
            ssl_throw( "BIO_new(BIO_s_mem)" );
        }

        SSL_set_bio( ssl_, in_, out_ );
        SSL_set_accept_state( ssl_ );
        SSL_do_handshake( ssl_ );
    }

    ~hello_ssl_service_impl( )
    {
        if( ctx_ ) {
            SSL_CTX_free( ctx_ );
        }

        if( ssl_ ) {
            SSL_free( ssl_ );
        }

//        if( in_ ) {
//            BIO_free(in_);
//        }

//        if( out_ ) {
//            BIO_free( out_ );
//        }
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

