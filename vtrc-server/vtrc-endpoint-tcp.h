#ifndef VTRC_ENDPOINT_TCP_H
#define VTRC_ENDPOINT_TCP_H

#include <string>

namespace vtrc { namespace server {

    struct endpoint_iface;
    class application;

    namespace endpoints { namespace tcp {
        endpoint_iface *create( application &app,
                                const std::string &address,
                                unsigned short service );
    }}

}}

#endif // VTRCENDPOINTTCP_H
