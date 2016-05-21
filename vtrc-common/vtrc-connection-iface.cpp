
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-protocol-layer.h"

namespace vtrc { namespace common {

    namespace ll = lowlevel;

    const ll::protocol_layer_iface *connection_iface::get_lowlevel( ) const
    {
        return get_protocol( ).get_lowlevel( );
    }

}}

