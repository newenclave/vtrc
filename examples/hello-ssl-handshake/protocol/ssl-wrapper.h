#ifndef SSL_WRAPPER_H
#define SSL_WRAPPER_H

#include <string>
#include "openssl/ssl.h"

class ssl_wrapper {

    SSL     *ssl_;
    SSL_CTX *ctx_;
    BIO     *in_;
    BIO     *out_;
    const SSL_METHOD *meth_;

protected:
    ssl_wrapper( const SSL_METHOD *meth )
        :ssl_(NULL)
        ,ctx_(NULL)
        ,in_(NULL)
        ,out_(NULL)
        ,meth_(meth)
    { }

public:

    virtual ~ssl_wrapper( )
    {
        if( ssl_ ) {
            SSL_free( ssl_ );
        }
        if( ctx_ ) {
            SSL_CTX_free( ctx_ );
        }
    }

    void init( )
    {
        ctx_ = SSL_CTX_new (meth_);
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
        SSL_set_bio( ssl_, in_, out_ );

        init_ssl( );
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

    BIO *get_in( )
    {
        return in_;
    }

    BIO *get_out( )
    {
        return out_;
    }

    bool init_finished( ) const
    {
        return SSL_is_init_finished(ssl_);
    }

    std::string do_handshake( const std::string &data )
    {
        return do_handshake( data.data( ), data.size( ) );
    }

    std::string do_handshake(  )
    {
        return do_handshake( NULL, 0 );
    }

    std::string do_handshake( const char *data, size_t length )
    {
        if( length ) {
            (void)BIO_reset( in_ );
            if( BIO_write( in_, data, length ) < 0 ) {
                ssl_throw( "do_handshake -> BIO_write" );
            }
        }

        int n = SSL_do_handshake( ssl_ );

        if( n <= 0 ) {
            int err = SSL_get_error( ssl_, n );
            if( err == SSL_ERROR_WANT_READ ) {
            } else if( err == SSL_ERROR_WANT_WRITE ) {
            } else if( err == SSL_ERROR_NONE ) {
            } else {
              ssl_throw( "do_handshake" );
            }
        }
        return read_bio( out_ );
    }

    int in_write( const std::string &data )
    {
        return in_write( data.c_str( ), data.size( ) );
    }

    int in_write( const char *data, size_t length )
    {
        int n = BIO_write( in_, data, length );
        if( n < 0 ) {
            ssl_throw( "BIO_write" );
        }
        return n;
    }

    std::string out_read(  )
    {
        return read_bio( out_ );
    }

    size_t write( const std::string &data )
    {
        int n = SSL_write( ssl_, data.c_str( ), data.size( ) );
        if( n < 0 ) {
            ssl_throw( "SSL_write" );
        }
        return n;
    }

    void write_all( const std::string &data )
    {
        write_all( data.c_str( ), data.size( ) );
    }

    void write_all( const char *data, size_t length )
    {
        size_t blen = 0;
        while( blen < length ) {
            int n = SSL_write( ssl_, &data[blen], length - blen );
            if( n < 0 ) {
                ssl_throw( "SSL_write" );
            }
            blen += n;
        }
    }

    std::string read( size_t max )
    {
        std::string res(max, 0);
        int n = SSL_read( ssl_, &res[0], res.size( ) );
        if( n < 0 ) {
            ssl_throw( "SSL_read" );
        }
        res.resize( n );
        return res;
    }

    std::string read_all( )
    {
        char *wdata;
        size_t wlength = BIO_get_mem_data( in_, &wdata );
        std::string res( wlength, '\0' );

        size_t len = 0;

        int n   = 0;

        while( true ) {
            n = SSL_read( ssl_, &res[len], res.size( ) - len);
            if( n < 0 ) {
                if( SSL_get_error( ssl_, n ) == SSL_ERROR_WANT_READ ) {
                    break;
                }
                ssl_throw( "SSL_read" );
            }
            len += n;
        }
        res.resize( len );
        return res;
    }

    std::string encrypt( const std::string &data )
    {
        return encrypt( data.c_str( ), data.size( ) );
    }

    std::string encrypt( const char *data, size_t length )
    {
        write_all( data, length );
        return read_bio( out_ );
    }

    std::string decrypt( const std::string &data )
    {
        return decrypt( data.c_str( ), data.size( ) );
    }

    std::string decrypt( const char *data, size_t length )
    {
        if( 0 == length ) {
            return std::string( );
        }

        int n = BIO_write( in_, data, length );
        if( n < 0 ) {
            ssl_throw( "BIO_write" );
        }
        return read_all( );
//        char *wdata;
//        size_t wlength = BIO_get_mem_data( in_, &wdata );
//        std::string res( wlength, '\0' );

//        n = SSL_read( ssl_, &res[0], res.size( ));

//        if( n < 0 ) {
//            ssl_throw( "SSL_read" );
//        }

//        res.resize( n );
//        return res;
    }

    protected:

        std::string read_bio( BIO *b )
        {
            char *data;
            size_t length = BIO_get_mem_data( b, &data );
            if( length ) {
                std::string res( length, 0 );
                BIO_read( b, &res[0], length );
                return res;
            } else {
                return std::string( );
            }
        }

        void ssl_throw( const char *add )
        {
            std::string err(add);
            err += ": ";
            size_t final = err.size( );
            err.resize( final + 1024 );
            int error_code = ERR_get_error( );
            ERR_error_string_n( error_code, &err[final], err.size( ) - final );
            err.resize( strlen( err.c_str( ) ) );
            throw std::runtime_error( err );
        }
};

class ssl_wrapper_server: public ssl_wrapper {
public:
    ssl_wrapper_server(const SSL_METHOD *meth)
        :ssl_wrapper(meth)
    { }
private:
    void init_ssl( )
    {
        SSL_set_accept_state( get_ssl( ) );
    }
};

class ssl_wrapper_client: public ssl_wrapper {
public:
    ssl_wrapper_client(const SSL_METHOD *meth)
        :ssl_wrapper(meth)
    { }
private:
    void init_ssl( )
    {
        SSL_set_connect_state(  get_ssl( ) );
    }
};

#endif // SSL_WRAPPER_H

