#ifndef VTRC_CHANNELS_H
#define VTRC_CHANNELS_H

#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-common/vtrc-connection-iface.h"

namespace vtrc { namespace client { namespace channels {

    common::rpc_channel *create( common::connection_iface_sptr c );
    common::rpc_channel *create( common::connection_iface_sptr c,
                                 unsigned flags );


}}}

#endif // VTRCCHANNELS_H

