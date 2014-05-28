
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

    void ping_impl( interface &iface, unsigned payload )
    {
        time_point start = chrono::high_resolution_clock::now( );

        std::cout << "Send ping with " << payload << " bytes as payload...";

        iface.ping( payload );

        time_point stop = chrono::high_resolution_clock::now( );

        std::cout << "ok; " << cast(stop - start)
                  << " microseconds\n";
    }

    void ping(interface &iface, bool flood, unsigned count, unsigned payload)
    {
        std::cout << "Start pinging...\n";
        while( count-- ) {
            ping_impl( iface, payload );
            if( !flood && count ) {
                this_thread::sleep_for( chrono::seconds( 1 ) );
            }
        }
    }

}

