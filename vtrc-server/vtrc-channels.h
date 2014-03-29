#ifndef VTRC_UNICAST_CHANNELS_H
#define VTRC_UNICAST_CHANNELS_H

#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-connection-list.h"

namespace vtrc { namespace server {

namespace channels {

namespace unicast {

    common::rpc_channel
        *create_event_channel( common::connection_iface_sptr c,
                                          bool disable_wait = false);

    common::rpc_channel
        *create_callback_channel( common::connection_iface_sptr c,
                                             bool disable_wait = false);
}

namespace broadcast {

    common::rpc_channel *create_event_channel(
                            vtrc::shared_ptr<common::connection_list> cl,
                            common::connection_iface_sptr c );

    common::rpc_channel *create_event_channel(
                            vtrc::shared_ptr<common::connection_list> cl );

}

}

}}

#endif // VTRC_UNICAST_CHANNELS_H
