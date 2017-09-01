#ifndef VTRC_COMMON_TIMER_PERIODICAL_H
#define VTRC_COMMON_TIMER_PERIODICAL_H

#include "vtrc/common/config/vtrc-asio.h"
#include "vtrc/common/config/vtrc-system.h"
#include "vtrc/common/config/vtrc-function.h"
#include "vtrc/common/config/vtrc-bind.h"
#include "vtrc/common/config/vtrc-stdint.h"
#include "vtrc/common/config/vtrc-mutex.h"

#include "vtrc/common/timers/deadline.h"

namespace vtrc { namespace common {  namespace timers {

    class  periodical {

    public:
        typedef VTRC_SYSTEM::error_code error_code;
        typedef VTRC_ASIO::io_service   io_service;

    private:

        typedef chrono::steady_clock::duration  duration_type;

        typedef vtrc::function<void (const error_code &)> hdl_type;

        void handler( const error_code &err, hdl_type userhdl )
        {
            userhdl( err );
            if( !err && enabled( ) ) {
                call_impl( userhdl );
            }
        }

        void call_impl( hdl_type &hdl )
        {
            namespace ph = vtrc::placeholders;
            VTRC_SYSTEM::error_code err;
            timer_.expires_from_now( duration_, err );
            if( !err ) {
                hdl_type uhdl = vtrc::bind( &periodical::handler, this,
                                            ph::_1, hdl );
                timer_.async_wait( uhdl );
            } else {
                hdl( err );
            }
        }

        void set_enabled( bool val )
        {
            vtrc::lock_guard<vtrc::mutex> lck(lock_);
            enabled_ = val;
        }

        periodical ( periodical& );
        periodical &operator = ( periodical& );

    public:

        periodical( io_service &ios )
            :timer_(ios)
            ,enabled_(false)
        { }

        template <typename Duration>
        void set_period( Duration dur )
        {
            duration_ = vtrc::chrono::duration_cast<duration_type>(dur);
        }

        void cancel( )
        {
            vtrc::lock_guard<vtrc::mutex> lck(lock_);
            if( enabled_ ) {
                enabled_ = false;
                timer_.cancel( );
            }
        }

        bool enabled( ) const
        {
            vtrc::lock_guard<vtrc::mutex> lck(lock_);
            return enabled_;
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
        void call( Handler hdl, Duration dur )
        {
            namespace ph = vtrc::placeholders;
            duration_ = vtrc::chrono::duration_cast<duration_type>(dur);
            timer_.expires_from_now( duration_ );
            set_enabled( true );
            hdl_type uhdl = vtrc::bind( &periodical::handler, this,
                                        ph::_1, hdl );
            timer_.async_wait( uhdl );
        }

        template <typename Handler, typename Duration>
        void call( Handler hdl, Duration dur, error_code &err )
        {
            namespace ph = vtrc::placeholders;
            duration_ = vtrc::chrono::duration_cast<duration_type>(dur);
            timer_.expires_from_now( duration_, err );
            if( !err ) {
                set_enabled( true );
                hdl_type uhdl = vtrc::bind( &periodical::handler, this,
                                            ph::_1, hdl );
                timer_.async_wait( uhdl );
            }
        }
    private:
        deadline            timer_;
        duration_type       duration_;
        mutable vtrc::mutex lock_;
        bool                enabled_;
    };

}}}

#endif // PERIODICAL_H
