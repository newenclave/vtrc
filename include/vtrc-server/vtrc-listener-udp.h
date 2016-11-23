#ifndef VTRC_LISTENER_UDP_H
#define VTRC_LISTENER_UDP_H

#include <string>

#include "vtrc-listener.h"

namespace vtrc { namespace server {

    class application;

    namespace listeners { namespace udp {

        /// tcp_nodelay sets TCP_NODELAY socket option for every connection
        listener_sptr create( application &app,
                              const std::string &address,
                              unsigned short service );

        listener_sptr create( application &app,
                              const rpc::session_options &opts,
                              const std::string &address,
                              unsigned short service );
    }}

}}

#endif // VTRCENDPOINTTCP_H
