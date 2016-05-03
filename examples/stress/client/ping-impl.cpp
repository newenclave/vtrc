#include <iostream>

#include "ping-impl.h"

#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-delayed-call.h"

#include "vtrc-chrono.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"
#include "vtrc-thread.h"

#if defined( _WIN32 )
#define sleep_ Sleep /// milliseconds
#define MILLISECONDS( x ) x
#else
#define sleep_ usleep /// microseconds
#define MILLISECONDS( x ) ((x) * 1000)
#endif

namespace stress {

    using namespace vtrc;
    typedef chrono::high_resolution_clock::time_point time_point;
    using namespace vtrc::common;

    template <typename TT>
    size_t cast( const TT& point )
    {
        return chrono::duration_cast<chrono::microseconds>( point ).count( );
    }

    size_t ping_impl( interface &iface, unsigned payload )
    {

        //std::cout << "Send ping with " << payload << " bytes as payload...";

        time_point start = chrono::high_resolution_clock::now( );

        iface.ping( payload );

        time_point stop = chrono::high_resolution_clock::now( );

        size_t to = cast(stop - start);
//        std::cout << "ok; " << to
//                  << " microseconds\n";
        return to;
    }

    void ping( interface &iface, bool flood, unsigned count, unsigned payload )
    {
        std::cout << "Start pinging...\n";
        size_t   res = 0;
        const unsigned counter = count;
        time_point start = chrono::high_resolution_clock::now( );

        while( count-- ) {

            res += ping_impl( iface, payload );

            if( !flood && count ) {
                sleep_( MILLISECONDS( 1000 ) );
            }
        }

        time_point stop = chrono::high_resolution_clock::now( );
        size_t to = cast( stop - start );

        std::cout << "Avarage: "
                  << ( res / counter )
                  << " (" << (to / counter) << ") "
                  << " microseconds\n";
    }

}

