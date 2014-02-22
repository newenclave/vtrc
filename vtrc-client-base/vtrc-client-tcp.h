#ifndef VTRC_CLIENT_TCP_H
#define VTRC_CLIENT_TCP_H

#include <boost/function.hpp>

#include "vtrc-common/vtrc-transport-tcp.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc { namespace client {

    class vtrc_client;

    class client_tcp: public common::transport_tcp {

        struct client_tcp_impl;
        client_tcp_impl  *impl_;

    public:

        typedef boost::function <
                    void (const boost::system::error_code &)
                > closure_type;

        client_tcp( boost::asio::io_service &ios, vtrc_client *client );
        ~client_tcp( );

        void connect( const std::string &address,
                      const std::string &service );
        void async_connect( const std::string &address,
                            const std::string &service,
                            closure_type closure );
        void on_write_error( const boost::system::error_code &err );
        void send_message( const char *data, size_t length );

    private:
        std::string prepare_for_write( const char *data, size_t len );

    };
}}

#endif // VTRCCLIENTTCP_H
