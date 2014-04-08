#ifndef VTRC_ENDPOINT_UNIX_LOCAL_H
#define VTRC_ENDPOINT_UNIX_LOCAL_H

#ifndef  _WIN32

#include <string>

#include "vtrc-endpoint-iface.h"

namespace vtrc { namespace server {

    class application;

    namespace endpoints { namespace unix_local {
        endpoint_iface *create( application &app, const std::string &name );
        endpoint_iface *create( application &app, const endpoint_options &opts,
                                const std::string &name );
    }}

}}

#endif

#endif // VTRC_ENDPOINT_UNIX_LOCAL_H

