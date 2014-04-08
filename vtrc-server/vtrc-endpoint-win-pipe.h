#ifndef VTRC_ENDPOINT_WIN_PIPE_H
#define VTRC_ENDPOINT_WIN_PIPE_H

#ifdef  _WIN32

#include <string>

#include "vtrc-endpoint-iface.h"

namespace vtrc { namespace server {

    class application;

    namespace endpoints { namespace win_pipe {
        endpoint_iface *create( application &app, const std::string &name );
        endpoint_iface *create( application &app, const endpoint_options &opts,
                                const std::string &name );
    }}

}}


#endif

#endif
