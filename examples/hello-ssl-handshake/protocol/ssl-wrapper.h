#ifndef SSL_WRAPPER_H
#define SSL_WRAPPER_H

#include <string>
#include "openssl/ssl.h"

class ssw_wrapper {

    SSL     *ssl_;
    SSL_CTX *ctx_;
    BIO     *in_;
    BIO     *out_;
    const SSL_METHOD *meth_;

protected:
    ssw_wrapper( const SSL_METHOD *meth )
        :ssl_(NULL)
        ,ctx_(NULL)
        ,in_(NULL)
        ,out_(NULL)
        ,meth_(meth)
    { }

public:

    virtual ~ssw_wrapper( ) { }

    void init( )
    {
        const SSL_METHOD *meth = TLSv1_2_server_method( );
        ctx_ = SSL_CTX_new (meth);
        if ( !ctx_ ) {
            ssl_throw( "SSL_CTX_new" );
        }
        init_context( );

        ssl_ = SSL_new( ctx_ );
        if( !ssl_ ) {
            ssl_throw( "SSL_new" );
        }

        in_  = BIO_new( BIO_s_mem( ) );
        out_ = BIO_new( BIO_s_mem( ) );
        if( !in_ || !out_ ) {
            ssl_throw( "BIO_new(BIO_s_mem)" );
        }
    }

    virtual void init_context( ) = 0;
    virtual void init_ssl( ) = 0;

    SSL *get_ssl( )
    {
        return ssl_;
    }

    const SSL *get_ssl( ) const
    {
        return ssl_;
    }

    SSL_CTX *get_context( )
    {
        return ctx_;
    }

    const SSL_CTX *get_context( ) const
    {
        return ctx_;
    }

    bool init_finished( ) const
    {
        return !!SSL_is_init_finished(ssl_);
    }

    std::string do_handshake( const std::string &data )
    {
        return do_handshake( data.data( ), data.size( ) );
    }

    std::string do_handshake( const char *data, size_t length )
    {
        if( length ) {
            BIO_reset( in_ );
            if( BIO_write( in_, data, length ) < 0 ) {
                ssl_throw( "do_handshake -> BIO_write" );
            }
        }

        int n = SSL_do_handshake( ssl_ );

        if( n <= 0 ) {
            int err = SSL_get_error( ssl, n );
            if( err == SSL_ERROR_WANT_READ ) {
            } else if( err == SSL_ERROR_WANT_WRITE ) {
            } else if( err == SSL_ERROR_NONE ) {
            } else {
              ssl_throw( "do_handshake" );
            }
        }
        char *wdata;
        size_t wlength = BIO_get_mem_data( out_, &wdata );
        return std::string( wdata, wdata + wlength );
    }

    protected:
        void ssl_throw( const char *add )
        {
            std::string err(add);
            err += ": ";
            size_t final = err.size( );
            err.resize( final + 1024 );
            ERR_error_string_n( ERR_get_error( ),
                                &err[final], err.size( ) - final );
            throw std::runtime_error( err );
        }
};

#endif // SSL_WRAPPER_H

