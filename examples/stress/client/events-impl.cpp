#include <stack>

#include "events-impl.h"

#include "protocol/stress.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

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

        time_point  last_event_point_;
        unsigned    payload_;

    public:
        events_impl( vtrc::shared_ptr<vtrc::client::vtrc_client> c,
                     unsigned payload )
            :client_(c)
            ,last_event_point_(high_resolution_clock::now( ))
            ,payload_(payload)
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

        std::string get_stack( )
        {
            client::vtrc_client_sptr locked( client_.lock( ) );

            if( locked ) { /// just in case;

                const common::call_context *cc(locked->get_call_context( ));
                size_t depth = cc->depth( );
                std::ostringstream oss;
                oss << "depth: "        << depth        << "; "
                    << "client depth: " << (depth >> 1)  << "; "
                    << "server depth: " << (depth >> 1);
                return oss.str( );
            }

            return "<>failed<>?";
        }

        std::string get_stack1( )
        {
            client::vtrc_client_sptr locked( client_.lock( ) );

            if( locked ) { /// just in case;

                const common::call_context *cc(locked->get_call_context( ));
                std::ostringstream oss;
                bool from_server = true;
                while( cc ) {
                    oss << (from_server ? ">" : "<");
                    from_server = !from_server;
                    cc = cc->next( );
                }
                return oss.str( );
            }

            return "<>failed<>?";
        }

        std::string get_stack2( )
        {
            client::vtrc_client_sptr locked( client_.lock( ) );

            if( locked ) { /// just in case;

                const common::call_context *cc(locked->get_call_context( ));
                std::ostringstream oss;
                bool from_server = true;
                while( cc ) {
                    oss << (from_server ? "S>" : "C<") << ":"
                        << cc->get_lowlevel_message( )->call( ).service_id( )
                        << "::"
                        << cc->get_lowlevel_message( )->call( ).method_id( );
                    cc = cc->next( );
                    from_server = !from_server;
                    if( cc ) {
                        oss << "->";
                    }
                }
                return oss.str( );
            }

            return "<>failed<>?";
        }

        void recursive_callback(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::recursive_call_req* request,
                 ::vtrc_example::recursive_call_res* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder( done );

            std::string stack(get_stack( ));

            std::cout << "[IN ] balance: " << request->balance( )
                      << "; stack: " << stack
                      << "; thread: " << vtrc::this_thread::get_id( )
                      << "\n";

            client::vtrc_client_sptr locked(client_.lock( ));

            if( locked ) {
                vtrc::shared_ptr<interface> impl(
                    create_stress_client(locked,
                        common::rpc_channel::USE_CONTEXT_CALL));

                impl->recursive_call( request->balance( ) - 1, payload_ );

            }

            if( request->balance( ) == 1 ) {
                std::cout << "============= EXIT =============\n";
            }

            std::cout << "[OUT] balance: " << request->balance( )
                      << "; stack: " << stack
                      << "; thread: " << vtrc::this_thread::get_id( )
                      << "\n";
        }


    };
}

    gpb::Service *create_events( vtrc::shared_ptr<client::vtrc_client> c,
                                 unsigned payload)
    {
        return new events_impl( c, payload );
    }

}
