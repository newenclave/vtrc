#include "vtrc/client/ssl.h"

#if VTRC_OPENSSL_ENABLED

#include <iostream>

#include "vtrc-asio.h"

#include "vtrc/client/stream-impl.h"

namespace vtrc { namespace client {

    namespace {
        namespace basio = VTRC_ASIO;
        namespace bssl  = VTRC_ASIO::ssl;
        namespace bsys  = VTRC_SYSTEM;
        namespace ph    = vtrc::placeholders;

        typedef basio::ip::tcp::socket           lowest_socket_type;
        typedef bssl::stream<lowest_socket_type> socket_type;
        typedef client_stream_impl<client_ssl, socket_type>  super_type;

        bool verify_certificate( bool prever, bssl::verify_context& /*ctx*/ )
        {
            return prever;
        }
    }

    struct client_ssl::impl: public super_type {

        typedef impl this_type;

        bool                            no_delay_;
        vtrc::shared_ptr<bssl::context> context_;
        verify_callback_type            vcb_;

        impl( VTRC_ASIO::io_service &ios,
              client::base *client, protocol_signals *callbacks,
              bool nodelay )
            :super_type(ios, client, callbacks, 4096)
            ,no_delay_(nodelay)
            ,vcb_(verify_certificate)
        { }

        bool verify_certificate_( bool prever, bssl::verify_context& ctx )
        {
            // The verify callback can be used to check whether the
            // certificate that is being presented is valid for the peer.
            // For example, RFC 2818 describes the steps involved in doing this
            // for HTTPS.
            // Consult the OpenSSL documentation for more details.
            // Note that the callback is called once for each certificate in the
            // certificate chain, starting from the root certificate authority.

            // In this example we will simply print the
            // certificate's subject name.

//            char subject_name[512];
//            X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle( ));
//            X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

            //std::cout << "Verifying " << subject_name << "\n";

            return prever;
        }

        void connect( const std::string &address,
                      unsigned short     service )
        {
            basio::ip::tcp::endpoint ep
                    (basio::ip::address::from_string(address), service );

            get_socket( ).set_verify_mode( bssl::verify_peer );
            get_socket( ).set_verify_callback( vcb_ );

            get_socket( ).lowest_layer( ).connect( ep );
            get_socket( ).handshake( bssl::stream_base::client );
            connection_setup( );
            start_reading( );
        }

        void set_verify_callback( verify_callback_type verify_callback )
        {
            vcb_ = verify_callback;
        }

        void connection_setup( )
        {
            if( no_delay_ ) {
                get_parent( )->set_no_delay( true );
            }
        }

        void handle_connect( const bsys::error_code& err,
                             common::system_closure_type closure,
                             common::connection_iface_sptr inst )
        {

            if ( !err ) {
                get_socket( ).async_handshake( bssl::stream_base::client,
                        vtrc::bind( &impl::handle_handshake, this,
                                    ph::error, closure, inst ) );
            } else {
                closure( err );
            }
        }

        void handle_handshake( const bsys::error_code& err,
                               common::system_closure_type closure,
                               common::connection_iface_sptr /*inst*/ )
        {
            if( !err ) {
                connection_setup( );
                start_reading( );
            }
            closure( err );
        }


        void async_connect( const std::string &address,
                            unsigned short     service,
                            common::system_closure_type closure )
        {
            basio::ip::tcp::endpoint ep
                    (basio::ip::address::from_string(address), service );

            get_socket( ).set_verify_mode( bssl::verify_peer );
            get_socket( ).set_verify_callback( vcb_ );

            get_socket( ).lowest_layer( ).async_connect( ep,
                    vtrc::bind( &this_type::handle_connect, this,
                                 ph::error, closure,
                                 parent_->shared_from_this( )) );
        }
    };

    static vtrc::shared_ptr<socket_type> create_socket( basio::io_service &ios,
                                                        bssl::context& ctx )
    {
        vtrc::shared_ptr<socket_type> inst( new socket_type(ios, ctx) );
        return inst;
        //return vtrc::make_shared<socket_type>(vtrc::ref(ios), vtrc::ref(ctx));
    }

    client_ssl::client_ssl( VTRC_ASIO::io_service &ios,
                            client::base *client,
                            vtrc::shared_ptr<bssl::context> ctx,
                            protocol_signals *callbacks,
                            bool tcp_nodelay )
        :common::transport_ssl(create_socket(ios, *ctx))
        ,impl_(new impl(ios, client, callbacks, tcp_nodelay ))
    {
        impl_->context_ = ctx;
        impl_->set_parent( this );
    }

    vtrc::shared_ptr<client_ssl> client_ssl::create( client::base *client,
                                        protocol_signals *callbacks,
                                        const std::string &verify_file,
                                        bool tcp_nodelay)
    {
        vtrc::shared_ptr<bssl::context> ctx
                (vtrc::make_shared<bssl::context>(bssl::context::sslv23 ) );
        ctx->load_verify_file( verify_file );
        vtrc::shared_ptr<client_ssl> new_inst
                    (new client_ssl( client->get_io_service( ),
                                     client, ctx,
                                     callbacks, tcp_nodelay ));
        new_inst->init( );
        return new_inst;
    }

    client_ssl::~client_ssl( )
    {
        delete impl_;
    }

    void client_ssl::connect( const std::string &address,
                              unsigned short service )
    {
        impl_->connect( address, service );
    }

    void client_ssl::set_verify_callback(verify_callback_type verify_callback)
    {
        impl_->set_verify_callback( verify_callback );
    }


    void client_ssl::async_connect(const std::string &address,
                                   unsigned short service,
                                   common::system_closure_type closure )
    {
        impl_->async_connect( address, service, closure );
    }

    void client_ssl::on_write_error( const VTRC_SYSTEM::error_code &err )
    {
        impl_->on_write_error( err );
        close( );
    }

    const common::call_context *client_ssl::get_call_context( ) const
    {
        return impl_->get_call_context( );
    }

    common::protocol_layer &client_ssl::get_protocol( )
    {
        return impl_->get_protocol( );
    }

    const common::protocol_layer &client_ssl::get_protocol( ) const
    {
        return impl_->get_protocol( );
    }


    common::environment &client_ssl::get_enviroment( )
    {
        return impl_->get_environment( );
    }

    void client_ssl::on_close( )
    {
        impl_->on_close( );
    }

    void client_ssl::init( )
    {
        impl_->init( );
    }

    bool client_ssl::active( ) const
    {
        return impl_->active( );
    }

    const std::string &client_ssl::id( ) const
    {
        return impl_->id( );
    }

    void client_ssl::raw_call_local (
                          vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                          common::empty_closure_type done )
    {
        impl_->raw_call_local( ll_mess, done );
    }

}}

#endif
