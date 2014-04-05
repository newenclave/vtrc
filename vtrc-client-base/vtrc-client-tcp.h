#ifndef VTRC_CLIENT_TCP_H
#define VTRC_CLIENT_TCP_H

#include "vtrc-common/vtrc-transport-tcp.h"
#include "vtrc-memory.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc {

namespace common {
    class protocol_layer;
}

namespace client {

    class vtrc_client;

    class client_tcp: public common::transport_tcp {

        struct impl;
        impl  *impl_;

        client_tcp( boost::asio::io_service &ios, vtrc_client *client );

    public:

        typedef vtrc::shared_ptr<client_tcp> shared_type;

        static shared_type create (
                            boost::asio::io_service &ios, vtrc_client *client );

        ~client_tcp( );
        void init( );
        bool active( ) const;

        void connect( const std::string &address,
                      const std::string &service );
        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type closure );
        void on_write_error( const boost::system::error_code &err );

        const common::call_context *get_call_context( ) const;
        common::protocol_layer &get_protocol( );

    private:
        void on_close( );
        std::string prepare_for_write( const char *data, size_t len );

    };
}}

#endif // VTRCCLIENTTCP_H
