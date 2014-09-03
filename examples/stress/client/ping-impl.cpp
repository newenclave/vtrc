#include <iostream>

#include "ping-impl.h"

#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-delayed-call.h"

#include "vtrc-chrono.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"
#include "vtrc-thread.h"


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

        std::cout << "Send ping with " << payload << " bytes as payload...";

        time_point start = chrono::high_resolution_clock::now( );

        iface.ping( payload );

        time_point stop = chrono::high_resolution_clock::now( );

        size_t to = cast(stop - start);
        std::cout << "ok; " << to
                  << " microseconds\n";
        return to;
    }

    void ping(interface &iface, bool flood, unsigned count, unsigned payload)
    {
        std::cout << "Start pinging...\n";
        size_t   res = 0;
        const unsigned counter = count;
        while( count-- ) {

            res += ping_impl( iface, payload );

            if( !flood && count ) {
                this_thread::sleep_for( chrono::seconds( 1 ) );
            }
        }

        std::cout << "Avarage TO: "
                  << ( res / counter )
                  << " microseconds\n";
    }

}

