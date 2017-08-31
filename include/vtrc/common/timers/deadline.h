#ifndef VTRC_COMMON_TIMERS_DEADLINE_H
#define VTRC_COMMON_TIMERS_DEADLINE_H

#include "vtrc/common/config/vtrc-chrono.h"
#include "vtrc/common/config/vtrc-asio.h"

namespace vtrc { namespace common {  namespace timers {
    typedef VTRC_ASIO::basic_waitable_timer<chrono::steady_clock> deadline;
}}}

#endif // DEADLINE_H
