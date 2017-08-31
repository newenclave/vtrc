#ifndef VTRC_COMMON_TIMER_ONCE_H
#define VTRC_COMMON_TIMER_ONCE_H

#include "vtrc/common/config/vtrc-asio.h"
#include "vtrc/common/config/vtrc-system.h"
#include "vtrc/common/timers/deadline.h"

namespace vtrc { namespace common {  namespace timers {

    class once { // NOT periodical

        deadline timer_;
        once( const once & );
        once &operator = ( const once & );

    public:

        typedef VTRC_SYSTEM::error_code error_code;

        once( VTRC_ASIO::io_service &ios )
            :timer_(ios)
        { }

        void cancel( )
        {
            timer_.cancel( );
        }

        deadline &timer( )
        {
            return timer_;
        }

        const deadline &timer( ) const
        {
            return timer_;
        }

        template <typename Handler, typename Duration>
        void call( Handler hdl, const Duration &dur )
        {
            timer_.expires_from_now( dur );
            timer_.async_wait( hdl );
        }

        template <typename Handler, typename Duration>
        void call( Handler hdl, const Duration &dur, error_code &err )
        {
            timer_.expires_from_now( dur, err );
            timer_.async_wait( hdl );
        }

    };

}}}

#endif // ONCE_H
