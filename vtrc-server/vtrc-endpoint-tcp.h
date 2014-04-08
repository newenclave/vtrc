#ifndef VTRC_ENDPOINT_TCP_H
#define VTRC_ENDPOINT_TCP_H

#include <string>

#include "vtrc-endpoint-iface.h"

namespace vtrc { namespace server {

    class application;

    namespace endpoints { namespace tcp {

        endpoint_iface *create( application &app,
                                const std::string &address,
                                unsigned short service );

        endpoint_iface *create( application &app, const endpoint_options &opts,
                                const std::string &address,
                                unsigned short service );
    }}

}}

#endif // VTRCENDPOINTTCP_H
