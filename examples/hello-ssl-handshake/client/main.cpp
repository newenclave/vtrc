#include <iostream>

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-stub-wrapper.h"

#include "protocol/hello.pb.h"

#include "boost/lexical_cast.hpp"

#include "openssl/ssl.h"
#include "openssl/err.h"

using namespace vtrc;

const std::string VERIFY_CSR = "../server.csr";
const std::string CERTF = "../server.crt";
#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 16
#endif
void hexdump(const void *mem, unsigned int len)
{
        unsigned int i, j;

        for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
        {
                /* print offset */
                if(i % HEXDUMP_COLS == 0)
                {
                        printf("0x%06x: ", i);
                }

                /* print hex data */
                if(i < len)
                {
                        printf("%02x ", 0xFF & ((char*)mem)[i]);
                }
                else /* end of block, just aligning for ASCII dump */
                {
                        printf("   ");
                }

                /* print ASCII dump */
                if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
                {
                        for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
                        {
                                if(j >= len) /* end of block, not really printing */
                                {
                                        putchar(' ');
                                }
                                else if(isprint(((char*)mem)[j])) /* printable char */
                                {
                                        putchar(0xFF & ((char*)mem)[j]);
                                }
                                else /* other char */
                                {
                                        putchar('.');
                                }
                        }
                        putchar('\n');
                }
        }
}

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
    std::cout << "Verify call: " << "\n";
    return 1;
}

void connect_handshake( stub_wrap &stub )
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
                std::cout << "More encrypted data required for handshake\n";
            } else if( err == SSL_ERROR_WANT_WRITE ) {
                std::cout << "Writting of data required for handshake\n";
            } else if( err == SSL_ERROR_NONE ) {
                std::cout << "No error but not accepted connection\n";
            } else {
              ssl_throw( "SSL_accept" );
            }
        }
        char *data;
        size_t length = BIO_get_mem_data( wbio, &data );
        hexdump(data, length);
        std::vector<char> raw_data(length);

        BIO_read( wbio, &raw_data[0], length );
        req.set_block( &raw_data[0], length );
        stub.call( &stub_type::send_block, &req, &res );

        BIO_write( rbio, res.block( ).c_str( ), res.block( ).size( ) );
        BIO_flush( rbio );
        BIO_flush( wbio );
    }

    std::string hello = "Hello, world!";
    SSL_write( ssl, hello.c_str( ), hello.size( ) );
    char *data;
    size_t length = BIO_get_mem_data( wbio, &data );

    req.set_block( data, length );
    stub.call( &stub_type::send_block, &req, &res );

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
