
#include "ping-impl.h"

#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-delayed-call.h"

#include "vtrc-chrono.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"


namespace stress {

    using namespace vtrc;
    typedef chrono::high_resolution_clock::time_point time_point;

    template <typename TT>
    chrono::microseconds cast( const TT& point )
    {
        return chrono::duration_cast<chrono::microseconds>( point );
    }

    void ping_impl( boost::system::error_code const &err,
                    interface &iface, unsigned count, unsigned payload,
                    vtrc::shared_ptr<common::delayed_call> dc,
                    vtrc::common::pool_pair &pp)
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
            pp.stop_all( );
            return;
        }

        if( --count == 0 ) {
            pp.stop_all( );
        } else {
            dc->call_from_now( vtrc::bind( ping_impl, _1,
                         vtrc::ref(iface), count, payload, dc,
                         vtrc::ref(pp) ), boost::posix_time::seconds( 1 ) );
        }
    }

    void ping(  interface &iface,
                unsigned count, unsigned payload,
                vtrc::common::pool_pair &pp)
    {

        std::cout << "Ping start:\n";
        vtrc::shared_ptr<common::delayed_call> dc
             (vtrc::make_shared<common::delayed_call>
                                ( vtrc::ref(pp.get_rpc_service( ))));

        dc->call_from_now( vtrc::bind( ping_impl, _1,
                     vtrc::ref(iface), count, payload, dc,
                     vtrc::ref(pp) ), boost::posix_time::seconds( 0 ) );

        pp.get_rpc_pool( ).attach( );
        std::cout << "Ping complete\n";
    }

}

