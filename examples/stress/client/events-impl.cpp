#include "events-impl.h"

#include "protocol/stress.pb.h"
#include "vtrc-client-base/vtrc-client.h"

#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-call-context.h"

#include "vtrc-chrono.h"
#include "vtrc-thread.h"


namespace stress {

    using namespace vtrc;
    namespace gpb = google::protobuf;

namespace {

    typedef vtrc::chrono::high_resolution_clock high_resolution_clock;
    typedef high_resolution_clock::time_point   time_point;

    size_t get_micro( const time_point &f, const time_point &l )
    {
        return chrono::duration_cast<chrono::microseconds>( l - f ).count( );
    }

    class events_impl: public vtrc_example::stress_events {

        client::vtrc_client_wptr client_;

        time_point last_event_point_;

    public:
        events_impl( vtrc::shared_ptr<vtrc::client::vtrc_client> c )
            :client_(c)
            ,last_event_point_(high_resolution_clock::now( ))
        { }
    private:

        void event(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::event_req* request,
                 ::vtrc_example::event_res* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );

            time_point this_event_point = high_resolution_clock::now( );

            std::string name( request->is_event( ) ? "Event" : "Callback" );

            std::cout << name << " id '" << request->id( ) << "'"
                      << "; ";
            if( request->id( ) > 0 ) {
                std::cout
                      << "previous: "
                      << get_micro( last_event_point_, this_event_point )
                      << " microseconds ago"
                      << "; thread " << this_thread::get_id( );
            }
            std::cout << "\n";

            last_event_point_ = this_event_point;
        }

    };
}

    gpb::Service *create_events( vtrc::shared_ptr<client::vtrc_client> c )
    {
        return new events_impl( c );
    }

}
