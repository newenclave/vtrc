#ifndef VTRC_MONOTONIC_TIMER_H
#define VTRC_MONOTONIC_TIMER_H

#include "vtrc-asio.h"
#include "vtrc-chrono.h"

namespace vtrc { namespace common { namespace timer {

    struct monotonic_traits {

        typedef boost::posix_time::milliseconds milliseconds;
        typedef boost::posix_time::microseconds microseconds;
        typedef boost::posix_time::seconds seconds;
        typedef boost::posix_time::minutes minutes;
        typedef boost::posix_time::hours hours;

        typedef vtrc::chrono::steady_clock::time_point time_type;
        typedef boost::posix_time::time_duration duration_type;

        static time_type now( )
        {
            return vtrc::chrono::steady_clock::now( );
        }

        static time_type add( const time_type& time,
                              const duration_type& duration )
        {
            return time +
                   vtrc::chrono::microseconds( duration.total_microseconds( ) );
        }

        static duration_type subtract( const time_type& timeLhs,
                                       const time_type& timeRhs )
        {
          vtrc::chrono::microseconds duration_us (
                vtrc::chrono::duration_cast<vtrc::chrono::microseconds>
                                                        (timeLhs - timeRhs));
          return microseconds( duration_us.count( ) );
        }

        static bool less_than( const time_type& timeLhs,
                               const time_type& timeRhs )
        {
            return timeLhs < timeRhs;
        }

        static duration_type to_posix_duration( const duration_type& duration )
        {
            return duration;
        }
    };

    typedef  VTRC_ASIO::basic_deadline_timer< vtrc::chrono::steady_clock,
                                             monotonic_traits > monotonic;

}}}

#endif // VTRCMONOTONICTIMER_H
