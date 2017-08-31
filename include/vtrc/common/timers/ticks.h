#ifndef VTRC_COMMON_TIMERS_TICKS_H
#define VTRC_COMMON_TIMERS_TICKS_H

#include "vtrc/common/config/vtrc-stdint.h"
#include "vtrc/common/config/vtrc-chrono.h"

namespace vtrc { namespace common {  namespace timers {

    template <typename DurationType = vtrc::chrono::steady_clock::duration>
    struct ticks {
        typedef DurationType duration_type;
        static vtrc::uint64_t now( )
        {
            typedef duration_type dt;
            using   vtrc::chrono::duration_cast;
            using   vtrc::chrono::high_resolution_clock;
            return  duration_cast<dt>( high_resolution_clock::now( )
                                      .time_since_epoch( ) ).count( );
        }
    };
}}}

#endif // TICKS_H
