#ifndef VTRC_ENDPOINT_TCP_H
#define VTRC_ENDPOINT_TCP_H

#include <string>

#include "vtrc-listener.h"

namespace vtrc { namespace server {

    class application;

    namespace listeners { namespace tcp {

        listener *create( application &app,
                                const std::string &address,
                                unsigned short service );

        listener *create( application &app, const listener_options &opts,
                                const std::string &address,
                                unsigned short service );
    }}

}}

#endif // VTRCENDPOINTTCP_H
