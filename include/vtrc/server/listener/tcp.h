#ifndef VTRC_LISTENER_TCP_H
#define VTRC_LISTENER_TCP_H

#include <string>

#include "vtrc/server/listener.h"

namespace vtrc { namespace server {

    class application;

    namespace listeners { namespace tcp {

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

#endif // VTRCENDPOINTTCP_H
