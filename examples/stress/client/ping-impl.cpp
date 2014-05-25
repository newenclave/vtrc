
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
    chrono::microseconds cast( const TT& point )
    {
        return chrono::duration_cast<chrono::microseconds>( point );
    }

    void ping_impl( interface &iface, unsigned payload )
    {
        time_point start = chrono::high_resolution_clock::now( );

        try {
            std::cout << "Send ping with " << payload << " bytes as payload...";
            iface.ping( payload );
            time_point stop = chrono::high_resolution_clock::now( );

            std::cout << "ok; " << cast(stop - start).count( )
                      << " microseconds\n";


        } catch ( const std::exception &ex )  {
            std::cout << "ping error: " << ex.what( ) << "\n";
            throw;
        }
    }

    void ping(interface &iface, unsigned count, unsigned payload)
    {
        std::cout << "Start pinging...\n";
        while( count-- ) {
            ping_impl( iface, payload );
            if( count ) this_thread::sleep_for( chrono::seconds( 1 ) );
        }
    }

}

