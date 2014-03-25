#ifndef VTRC_UNICAST_CHANNELS_H
#define VTRC_UNICAST_CHANNELS_H

#include "vtrc-common-channel.h"
#include "vtrc-common/vtrc-connection-iface.h"

namespace vtrc { namespace server {

namespace channels {

namespace unicast {
    common_channel *create_event_channel( common::connection_iface_sptr c );
    common_channel *create_callback_channel( common::connection_iface_sptr c );
}

}

}}

#endif // VTRC_UNICAST_CHANNELS_H
