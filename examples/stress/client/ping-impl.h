#ifndef PING_IMPL_H
#define PING_IMPL_H

#include "stress-iface.h"

namespace vtrc { namespace common {
    class pool_pair;
}}

namespace stress {

    void ping( stress::interface &iface, bool flood,
               unsigned count, unsigned payload );

}

#endif // PINGIMPL_H
