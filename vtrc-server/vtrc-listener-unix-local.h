#ifndef VTRC_ENDPOINT_UNIX_LOCAL_H
#define VTRC_ENDPOINT_UNIX_LOCAL_H

#ifndef  _WIN32

#include <string>

#include "vtrc-listener.h"

namespace vtrc { namespace server {

    class application;

    namespace listeners { namespace unix_local {
        listener *create( application &app, const std::string &name );
        listener *create( application &app, const listener_options &opts,
                                const std::string &name );
    }}

}}

#endif

#endif // VTRC_ENDPOINT_UNIX_LOCAL_H

