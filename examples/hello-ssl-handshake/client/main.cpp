#include <iostream>

#if !defined(NOMINMAX)
#define NOMINMAX // fckng Windows
#endif

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-random-device.h"

#include "protocol/hello.pb.h"

#include "boost/lexical_cast.hpp"

#include "openssl/err.h"
#include "openssl/rand.h"

#if defined(X509_NAME)
#undef X509_NAME // fckng MS
#endif

#include "openssl/ssl.h"

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

int verify_callback( X509_STORE_CTX *x509, void */*p*/ )
{
    char subject[512];
    int ver = X509_verify_cert(x509);
    if( ver <= 0 ) {
        int err = X509_STORE_CTX_get_error(x509);
        char errorbuf[1024];
        ERR_error_string_n(err, errorbuf, 1024);
        std::cout << "Cert is not valid. " << errorbuf << "\n";
    }
    X509 *cc = X509_STORE_CTX_get_current_cert( x509 );
    if( !cc ) {
        ssl_throw("ds");
    }
    X509_NAME *sn = X509_get_subject_name( cc );
    X509_NAME_oneline(sn, subject, 256);
    std::cout << "Verify call: " << subject << "\n";

    /// we don't care, cuz "Hello, world!"
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

        SSL_CTX_set_cert_verify_callback( get_context( ), verify_callback, 0 );

        int err = SSL_CTX_load_verify_locations( get_context( ),
                                                 CERTF.c_str( ), 0 );
        if( err == 0 ) {
            ssl_throw("SSL_CTX_load_verify_locations");
        }
    }
};

std::string send_data( stub_wrap &stub, my_ssl_wrapper &ssl,
                       const std::string &data, bool decode = true )
{
    howto::request_message  req;
    howto::response_message res;

    req.set_block( ssl.encrypt( data ) );
    //std::cout << "packed: " << req.block( ).size( ) << "\n";

    stub.call( &stub_type::send_block, &req, &res );
    std::string result;
    if( decode ) {
        result = ssl.decrypt( res.block( ) );
    } else {
        ssl.get_in( ).write( res.block( ) );
    }
//    std::cout << "result: " << result.size( ) << " "
//              << res.block( ).size( ) << "\n";
    return result;
}

void connect_handshake( stub_wrap &stub )
{
    my_ssl_wrapper ssl;
    ssl.init( );
    howto::request_message  req;
    howto::response_message res;
    std::string data;
    while (!ssl.init_finished( )) {

        std::string res_data = ssl.do_handshake( );

        if( !res_data.empty( ) ) {
            req.set_block( res_data );
            stub.call( &stub_type::send_block, &req, &res );
            data = res.block( );
            if( !data.empty( ) ) { //
                ssl.get_in( ).write( data.c_str( ), data.size( ) );
            }
        }
    }

    std::cout << "Init success!\n";

    int i = 0;

    std::cout << i++ << ": " << send_data( stub, ssl, "Hello, World1" )
              << "\n";
    std::cout << i++ << ": " << send_data( stub, ssl, "Hello, World2", false )
              << "\n";
    std::cout << i++ << ": " << send_data( stub, ssl, "Hello, World3" )
              << "\n";

    std::string test(41, '!');
    std::cout << i++ << ": " << send_data( stub, ssl, test ) << "\n";
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

    vtrc::common::random_device rd(true);
    std::string seed = rd.generate_block( 1024 );

    RAND_seed( seed.c_str( ), seed.size( ) );

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
