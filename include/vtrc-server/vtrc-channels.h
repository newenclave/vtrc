#ifndef VTRC_UNICAST_CHANNELS_H
#define VTRC_UNICAST_CHANNELS_H

#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-connection-list.h"

namespace vtrc { namespace server { namespace channels {

    namespace unicast {

        /// do not use client's call context
        common::rpc_channel
            *create_event_channel( common::connection_iface_sptr c,
                                              bool disable_wait = true );

        /// use client's call context
        common::rpc_channel
            *create_callback_channel( common::connection_iface_sptr c,
                                              bool disable_wait = false );

        common::rpc_channel *create( common::connection_iface_sptr c,
                            unsigned flags = common::rpc_channel::DEFAULT );

    } // unicast

    namespace broadcast {

        common::rpc_channel *create_event_channel(
                                vtrc::shared_ptr<common::connection_list> cl,
                                common::connection_iface_sptr c );

        common::rpc_channel *create_event_channel(
                                vtrc::shared_ptr<common::connection_list> cl );

    } // broadcast

}}}

#endif // VTRC_UNICAST_CHANNELS_H
