#ifndef VTRC_LISTENER_LOCAL_H
#define VTRC_LISTENER_LOCAL_H

#include <string>

#include "vtrc/server/listener.h"

namespace vtrc { namespace rpc {
    class session_options;
}}

namespace vtrc { namespace server {

    class application;

    namespace listeners { namespace local {
        listener_sptr create( application &app, const std::string &name );
        listener_sptr create( application &app,
                              const rpc::session_options &opts,
                              const std::string &name );
    }}

}}


#endif // VTRCLISTENERLOCAL_H
