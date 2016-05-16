#ifndef VTRC_DELAYED_CALL_H
#define VTRC_DELAYED_CALL_H

#include "vtrc-monotonic-timer.h"

VTRC_ASIO_FORWARD(
    class io_service;
)

namespace vtrc { namespace common {

    class delayed_call {

        timer::monotonic timer_;

    public:

        typedef timer::monotonic_traits::milliseconds   milliseconds;
        typedef timer::monotonic_traits::microseconds   microseconds;
        typedef timer::monotonic_traits::seconds        seconds;
        typedef timer::monotonic_traits::minutes        minutes;
        typedef timer::monotonic_traits::hours          hours;

        delayed_call( VTRCASIO::io_service &ios )
            :timer_(ios)
        { }

        timer::monotonic &timer( )
        {
            return timer_;
        }

        const timer::monotonic &timer( ) const
        {
            return timer_;
        }

        void cancel( )
        {
            timer_.cancel( );
        }

        template <typename FuncType, typename TimerType>
        void call_from_now( FuncType f, const TimerType &tt )
        {
            timer_.expires_from_now( tt );
            timer_.async_wait( f );
        }

    };

}}

#endif // VTRCDELAYEDCALL_H
