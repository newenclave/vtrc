#ifndef VTRC_MONOTONIC_TIMER_H
#define VTRC_MONOTONIC_TIMER_H

#include <boost/asio/steady_timer.hpp>

namespace vtrc { namespace common { namespace timer {

    struct monotonic_traits {

        typedef boost::chrono::steady_clock::time_point time_type;
        typedef boost::posix_time::time_duration duration_type;

        static time_type now()
        {
            return boost::chrono::steady_clock::now();
        }

        static time_type add(const time_type& time,
                             const duration_type& duration)
        {
            return time +
                   boost::chrono::microseconds(duration.total_microseconds());
        }

        static duration_type subtract(const time_type& timeLhs,
                                      const time_type& timeRhs)
        {
          boost::chrono::microseconds oChronoDuration_us(
                boost::chrono::duration_cast<boost::chrono::microseconds>
                                                        (timeLhs - timeRhs));
          return boost::posix_time::microseconds(oChronoDuration_us.count());
        }

        static bool less_than(const time_type& timeLhs,
                              const time_type& timeRhs)
        {
            return timeLhs < timeRhs;
        }

        static boost::posix_time::time_duration
                                to_posix_duration(const duration_type& duration)
        {
            return duration;
        }
    };

    typedef  boost::asio::basic_deadline_timer<
                boost::chrono::steady_clock,
                monotonic_traits > monotonic;

}}}

#endif // VTRCMONOTONICTIMER_H
