#ifndef VTRC_CLIENT_TCP_H
#define VTRC_CLIENT_TCP_H

#include <boost/function.hpp>

#include "vtrc-common/vtrc-transport-tcp.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc { namespace client {

    class client_tcp: public vtrc::common::transport_tcp {

        struct client_tcp_impl;
        client_tcp_impl  *impl_;

    public:

        client_tcp( boost::asio::io_service &ios );
        ~client_tcp( );

        void connect( const std::string &address,
                      const std::string &service );
        void async_connect( const std::string &address,
                            const std::string &service,
                            boost::function <
                                    void (const boost::system::error_code &)
                                >   closure );
        void on_write_error( const boost::system::error_code &err );
        void send_message( const char *data, size_t length );

    };
}}

#endif // VTRCCLIENTTCP_H
