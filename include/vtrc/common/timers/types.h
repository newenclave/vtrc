#ifndef VTRC_COMMON_TIMERS_TYPES_H
#define VTRC_COMMON_TIMERS_TYPES_H

#include "vtrc/common/config/vtrc-chrono.h"

namespace vtrc { namespace common {  namespace timers {
    typedef vtrc::chrono::nanoseconds   nanoseconds;
    typedef vtrc::chrono::microseconds  microseconds;
    typedef vtrc::chrono::milliseconds  milliseconds;
    typedef vtrc::chrono::seconds       seconds;
    typedef vtrc::chrono::minutes       minutes;
    typedef vtrc::chrono::hours         hours;
}}}

#endif // TYPES_H
