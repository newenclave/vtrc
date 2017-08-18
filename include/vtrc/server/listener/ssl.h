#ifndef VTRC_LISTENER_SSL_H
#define VTRC_LISTENER_SSL_H
#include "vtrc-general-config.h"

#if VTRC_OPENSSL_ENABLED

#include <string>

#include "vtrc/server/listener.h"
#include "vtrc-function.h"

namespace boost { namespace asio { namespace ssl {
    class context;
}}}

namespace vtrc { namespace server {

    class application;

    namespace ssl {
        typedef VTRC_ASIO::ssl::context context;
        typedef vtrc::function<void (context &)> setup_function_type;
    }

    namespace listeners { namespace tcp_ssl {

        /// tcp_nodelay sets TCP_NODELAY socket option for every connection
        listener_sptr create( application &app,
                              const std::string &address,
                              unsigned short service,
                              ssl::setup_function_type setup,
                              bool tcp_nodelay = false );

        listener_sptr create( application &app,
                              const rpc::session_options &opts,
                              const std::string &address,
                              unsigned short service,
                              ssl::setup_function_type setup,
                              bool tcp_nodelay = false );
    }}

}}

#endif // OPENSSL
#endif // VTRC_LISTENER_SSL_H
