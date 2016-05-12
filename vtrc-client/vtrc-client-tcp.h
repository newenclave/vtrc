#ifndef VTRC_CLIENT_TCP_H
#define VTRC_CLIENT_TCP_H

#include "vtrc-memory.h"
#include "vtrc-common/vtrc-transport-tcp.h"
#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-lowlevel-protocol-iface.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc {

namespace common {
    class protocol_layer;
}

namespace client {

    class   vtrc_client;
    struct  protocol_signals;

    class client_tcp: public common::transport_tcp {

        struct impl;
        impl  *impl_;

        client_tcp( boost::asio::io_service &ios,
                    vtrc_client *client,
                    protocol_signals *callbacks,
                    bool tcp_nodelay );

    public:

        typedef vtrc::shared_ptr<client_tcp> shared_type;

        typedef common::lowlevel::protocol_factory_type lowlevel_factory_type;

        static shared_type create ( boost::asio::io_service &ios,
                                    vtrc_client *client,
                                    protocol_signals *callbacks,
                                    lowlevel_factory_type factory,
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

        void on_write_error( const boost::system::error_code &err );

        const common::call_context *get_call_context( ) const;
        common::protocol_layer     &get_protocol( );
        common::enviroment         &get_enviroment( );

    private:
        void on_close( );
        std::string prepare_for_write( const char *data, size_t len );
        void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                              common::empty_closure_type done );

    };
}}

#endif // VTRCCLIENTTCP_H
