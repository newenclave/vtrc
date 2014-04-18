#ifndef VTRC_ENDPOINT_WIN_PIPE_H
#define VTRC_ENDPOINT_WIN_PIPE_H

#ifdef  _WIN32

#include <string>

#include "vtrc-listener.h"

namespace vtrc { namespace server {

    class application;

    namespace listeners { namespace win_pipe {
        listener *create( application &app, const std::string &name );
        listener *create( application &app, const listener_options &opts,
                                const std::string &name );
    }}

}}


#endif

#endif
