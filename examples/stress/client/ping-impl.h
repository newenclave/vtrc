#ifndef PING_IMPL_H
#define PING_IMPL_H

#include "stress-iface.h"

namespace vtrc { namespace common {
    class pool_pair;
}}

namespace stress {

    void ping( stress::interface &iface,
               unsigned count, unsigned payload,
               vtrc::common::pool_pair &pp );

}

#endif // PINGIMPL_H
