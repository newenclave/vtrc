#include <iostream>

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-stub-wrapper.h"

#include "protocol/hello.pb.h"

#include "boost/lexical_cast.hpp"

#include "openssl/ssl.h"
#include "openssl/err.h"

#include "../protocol/ssl-wrapper.h"

using namespace vtrc;

const std::string CERTF = "../server.crt";

namespace {

    typedef howto::hello_ssl_service_Stub   stub_type;
    typedef common::stub_wrapper<stub_type> stub_wrap;

    void on_connect( )
    {
        std::cout << "connect...";
    }
    void on_ready( )
    {
        std::cout << "ready...";
    }
    void on_disconnect( )
    {
        std::cout << "disconnect...";
    }

    void ssl_throw( const char *add )
    {
        std::string err(add);
        err += ": ";
        size_t final = err.size( );
        err.resize( final + 1024 );
        ERR_error_string_n(ERR_get_error( ), &err[final], err.size( ) - final );
        throw std::runtime_error( err );
    }
}

int verify_dont_fail_cb( X509_STORE_CTX *x509, void *p )
{
    char subject[512];
    int ver = X509_verify_cert(x509);
    if( !ver ) {
        std::cout << "Cert is not valid\n";
    }
    X509 *cc = X509_STORE_CTX_get_current_cert( x509 );
    if( !cc ) {
        ssl_throw("ds");
    }
    X509_NAME *sn = X509_get_subject_name( cc );
    X509_NAME_oneline(sn, subject, 256);
    std::cout << "Verify call: " << subject << "\n";
    return 1;
}

class my_ssl_wrapper: public ssl_wrapper_client {
public:
    my_ssl_wrapper( )
        :ssl_wrapper_client(TLSv1_2_client_method( ))
    { }
private:
    void init_context( )
    {
        SSL_CTX_set_mode( get_context( ), SSL_MODE_AUTO_RETRY           );
        SSL_CTX_set_mode( get_context( ), SSL_MODE_ENABLE_PARTIAL_WRITE );
        SSL_CTX_set_mode( get_context( ), SSL_MODE_RELEASE_BUFFERS      );

        SSL_CTX_set_cert_verify_callback( get_context( ), verify_dont_fail_cb, 0 );

        int err = SSL_CTX_load_verify_locations( get_context( ), CERTF.c_str( ), 0 );
        if( err == 0 ) {
            ssl_throw("SSL_CTX_load_verify_locations");
        }
    }
};


void connect_handshake( stub_wrap &stub )
{
    my_ssl_wrapper ssl;
    ssl.init( );
    howto::request_message  req;
    howto::response_message res;
    std::string data;
    while (!ssl.init_finished( )) {
        std::string res_data = ssl.do_handshake( "" );
        req.set_block( res_data );
        stub.call( &stub_type::send_block, &req, &res );
        data = res.block( );
        BIO_write( ssl.get_in( ), data.c_str( ), data.size( ) );
    }

    std::string hello = "Hello, world!";
    SSL_write( ssl.get_ssl( ), hello.c_str( ), hello.size( ) );
    char *wdata;
    size_t length = BIO_get_mem_data( ssl.get_out( ), &wdata );

    req.set_block( wdata, length );
    stub.call( &stub_type::send_block, &req, &res );

    BIO_write( ssl.get_in( ), res.block( ).c_str( ), res.block( ).size( ) );

    length = BIO_get_mem_data( ssl.get_out( ), &wdata );

    std::string result(1024, 0);
    int n = SSL_read( ssl.get_ssl( ), &result[0], result.size( ) );
    result.resize( n );

    std::cout << "response: " << result << " " << n << "\n";

//    data = "Hello, world!";
//    req.set_block( ssl.encrypt( data ) );
//    stub.call( &stub_type::send_block, &req, &res );
//    std::cout << "Response: ";
//    std::cout <<  res.block( ) << "\n";
//    std::cout <<  ssl.decrypt(res.block( )) << "\n";
}

void connect_handshake_( stub_wrap &stub )
{
    howto::request_message  req;
    howto::response_message res;

    SSL     *ssl;
    SSL_CTX *ctx = SSL_CTX_new(TLSv1_2_client_method( ));

    if( !ctx ) {
        ssl_throw("SSL_CTX_new");
    }

    SSL_CTX_set_mode( ctx, SSL_MODE_AUTO_RETRY           );
    SSL_CTX_set_mode( ctx, SSL_MODE_ENABLE_PARTIAL_WRITE );
    SSL_CTX_set_mode( ctx, SSL_MODE_RELEASE_BUFFERS      );

    SSL_CTX_set_cert_verify_callback( ctx, verify_dont_fail_cb, 0 );

    int err = SSL_CTX_load_verify_locations( ctx, CERTF.c_str( ), 0 );

    if( err == 0 ) {
        ssl_throw("SSL_CTX_load_verify_locations");
    }

    ssl = SSL_new( ctx );

    if( !ssl ) {
        ssl_throw("SSL_new");
    }

    BIO *rbio = BIO_new( BIO_s_mem( ) );
    BIO *wbio = BIO_new( BIO_s_mem( ) );

    if( !rbio || !wbio ) {
        ssl_throw("BIO_new(BIO_s_mem");
    }

    SSL_set_bio( ssl, rbio, wbio );
    SSL_set_connect_state( ssl );

    while (!SSL_is_init_finished(ssl)) {
        int n = SSL_do_handshake(ssl);
        if( n <= 0 ) {
            int err = SSL_get_error( ssl, n );
            if( err == SSL_ERROR_WANT_READ ) {
                //std::cout << "More encrypted data required for handshake\n";
            } else if( err == SSL_ERROR_WANT_WRITE ) {
                //std::cout << "Writting of data required for handshake\n";
            } else if( err == SSL_ERROR_NONE ) {
                //std::cout << "No error but not accepted connection\n";
            } else {
              ssl_throw( "SSL_accept" );
            }
        }
        char *data;
        size_t length = BIO_get_mem_data( wbio, &data );
        req.set_block( data, length );
        (void)BIO_reset( wbio );
        stub.call( &stub_type::send_block, &req, &res );

        BIO_write( rbio, res.block( ).c_str( ), res.block( ).size( ) );
    }

    std::string hello = "Hello, world!";
    SSL_write( ssl, hello.c_str( ), hello.size( ) );
    char *data;
    size_t length = BIO_get_mem_data( wbio, &data );

    req.set_block( data, length );
    stub.call( &stub_type::send_block, &req, &res );

    BIO_write( rbio, res.block( ).c_str( ), res.block( ).size( ) );

    length = BIO_get_mem_data( wbio, &data );

    std::string result(1024, 0);
    int n = SSL_read( ssl, &result[0], result.size( ) );
    result.resize( n );

    std::cout << "response: " << result << " " << n << "\n";
}

int main( int argc, const char **argv )
{
    common::thread_pool tp( 1 );

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

    try {

        client::vtrc_client_sptr cl =
                         client::vtrc_client::create( tp.get_io_service( ) );

        cl->on_connect_connect( on_connect );
        cl->on_ready_connect( on_ready );
        cl->on_disconnect_connect( on_disconnect );

        std::cout <<  "Connecting..." << std::endl;

        cl->connect( address, port );
        std::cout << "Ok" << std::endl;

        vtrc::unique_ptr<common::rpc_channel> channel(cl->create_channel( ));

        stub_wrap hello(channel.get( ));

        connect_handshake( hello );

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    tp.stop( );
    tp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}
