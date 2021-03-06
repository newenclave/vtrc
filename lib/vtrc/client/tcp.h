#ifndef VTRC_CLIENT_TCP_H
#define VTRC_CLIENT_TCP_H

#include "vtrc-memory.h"
#include "vtrc/common/transport/tcp.h"
#include "vtrc/common/closure.h"
#include "vtrc/common/lowlevel-protocol-iface.h"

VTRC_ASIO_FORWARD(
    class io_service;
)

namespace vtrc {

namespace common {
    class protocol_layer;
}

namespace client {

    class   base;
    struct  protocol_signals;

    class client_tcp: public common::transport_tcp {

        struct impl;
        impl  *impl_;

        client_tcp( VTRC_ASIO::io_service &ios,
                    client::base *client,
                    protocol_signals *callbacks,
                    bool tcp_nodelay );

    public:

        typedef vtrc::shared_ptr<client_tcp> shared_type;

        typedef common::lowlevel::protocol_factory_type lowlevel_factory_type;

        static shared_type create ( client::base *client,
                                    protocol_signals *callbacks,
                                    bool tcp_nodelay );

        ~client_tcp( );
        void init( );
        bool active( ) const;
        const std::string &id( ) const;
        std::string name( ) const;

        void connect( const std::string &address,
                      unsigned short     service );

        void async_connect(const std::string &address,
                           unsigned short service,
                           common::system_closure_type closure );

        void on_write_error( const VTRC_SYSTEM::error_code &err );

        const common::call_context  *get_call_context( ) const;
        common::protocol_layer       &get_protocol( );
        const common::protocol_layer &get_protocol( ) const;
        common::environment           &get_enviroment( );

    private:
        void on_close( );
        void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                              common::empty_closure_type done );

    };
}}

#endif // VTRCCLIENTTCP_H
