#ifndef VTRC_LISTENER_LOCAL_H
#define VTRC_LISTENER_LOCAL_H

#include <string>

#include "vtrc-listener.h"

namespace vtrc { namespace server {

    class application;

    namespace listeners { namespace local {
        listener_sptr create( application &app, const std::string &name );
        listener_sptr create( application &app, const listener_options &opts,
                                const std::string &name );
    }}

}}


#endif // VTRCLISTENERLOCAL_H
