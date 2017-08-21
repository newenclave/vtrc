#ifndef VTRC_CONNECTION_CLOSURE_H
#define VTRC_CONNECTION_CLOSURE_H

#include "vtrc-function.h"
#include "vtrc/common/connection-iface.h"

namespace vtrc { namespace server {

    typedef vtrc::function<
        void ( common::connection_iface *)
    > connection_close_closure;

}}

#endif // VTRC_CONNECTION_CLOSURE_H
