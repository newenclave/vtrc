#ifndef VTRC_LISTENER_SSL_H
#define VTRC_LISTENER_SSL_H
#include "vtrc-general-config.h"

#if VTRC_OPENSSL_ENABLED

#include <string>

#include "vtrc-listener.h"

namespace vtrc { namespace server {

    class application;

    namespace listeners { namespace tcp_ssl {

        /// tcp_nodelay sets TCP_NODELAY socket option for every connection
        listener_sptr create( application &app,
                              const std::string &address,
                              unsigned short service,
                              bool tcp_nodelay = false );

        listener_sptr create( application &app,
                              const rpc::session_options &opts,
                              const std::string &address,
                              unsigned short service,
                              bool tcp_nodelay = false );
    }}

}}

#endif // OPENSSL
#endif // VTRC_LISTENER_SSL_H
