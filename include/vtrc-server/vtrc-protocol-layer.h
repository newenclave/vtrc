#ifndef VTRC_PROTOCOL_LAYER_H
#define VTRC_PROTOCOL_LAYER_H

#include "vtrc-common/vtrc-protocol-iface.h"
#include "vtrc-common/vtrc-connection-iface.h"

namespace vtrc { namespace rpc {
    class session_options;
}}

namespace vtrc { namespace server {
    class application;
    namespace protocol {
        common::protocol_iface *create( application &app,
                                    common::connection_iface_sptr connection );
        common::protocol_iface *create( application &app,
                                    common::connection_iface_sptr connection,
                                    const rpc::session_options &opts );
    }
}}

#endif // VTRC_PROTOCOL_LAYER_H

