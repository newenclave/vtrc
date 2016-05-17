#ifndef VTRC_CLIENT_SSL_H
#define VTRC_CLIENT_SSL_H

#include "vtrc-general-config.h"

#if VTRC_OPENSSL_ENABLED

#include "vtrc-memory.h"
#include "vtrc-common/vtrc-transport-ssl.h"
#include "vtrc-common/vtrc-closure.h"

VTRC_ASIO_FORWARD(
    class io_service;
    namespace ssl {
        class context;
    }
)

namespace vtrc {

namespace common {
    class protocol_layer;
}

namespace client {

    class   vtrc_client;
    struct  protocol_signals;

    class client_ssl: public common::transport_ssl {

        struct impl;
        impl  *impl_;

        client_ssl( VTRC_ASIO::io_service &ios,
                    vtrc_client *client,
                    vtrc::shared_ptr<VTRC_ASIO::ssl::context> ctx,
                    protocol_signals *callbacks,
                    bool tcp_nodelay );

    public:

        typedef vtrc::function<
            bool ( bool, VTRC_ASIO::ssl::verify_context& )
        > verify_callback_type;

        typedef vtrc::shared_ptr<client_ssl> shared_type;

        static shared_type create ( VTRC_ASIO::io_service &ios,
                                    vtrc_client *client,
                                    protocol_signals *callbacks,
                                    const std::string &verify_file,
                                    bool tcp_nodelay );

        void set_verify_callback( verify_callback_type verify_callback );

        ~client_ssl( );
        void init( );
        bool active( ) const;
        const std::string &id( ) const;

        void connect( const std::string &address,
                      unsigned short     service );

        void async_connect(const std::string &address,
                           unsigned short service,
                           common::system_closure_type closure );

        void on_write_error( const VTRC_SYSTEM::error_code &err );

        const common::call_context *get_call_context( ) const;
        common::protocol_layer     &get_protocol( );
        common::enviroment         &get_enviroment( );

    private:

        void on_close( );
        void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                              common::empty_closure_type done );

    };
}}

#endif // OPENSSL
#endif // VTRC_CLIENT_SSL_H
