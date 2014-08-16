#include <iostream>

#include "stress-service-impl.h"
#include "protocol/stress.pb.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-server/vtrc-channels.h"
#include "vtrc-common/vtrc-stub-wrapper.h"

#include "google/protobuf/descriptor.h"

#include "stress-application.h"

namespace gpb = google::protobuf;
using namespace vtrc;

namespace  {

    typedef vtrc_example::stress_events_Stub stub_type;
    typedef common::stub_wrapper<stub_type>  stub_wrapper_type;

    using namespace server::channels;

    class stress_service_impl: public vtrc_example::stress_service {

        stress::application      &app_;
        common::connection_iface *c_;

    public:

        stress_service_impl( stress::application &app,
                             common::connection_iface *c )
            :app_(app)
            ,c_(c)
        { }

    private:

        void init(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::empty* request,
                 ::vtrc_example::empty* response,
                 ::google::protobuf::Closure* done)
        {
            /// do nothing
            common::closure_holder holder(done);
        }


        void ping(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::ping_req* request,
                 ::vtrc_example::ping_res* response,
                 ::google::protobuf::Closure* done)
        {
            /// do nothing
            common::closure_holder holder(done);
        }

        void generate_event(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::generate_events_req* request,
                 ::vtrc_example::generate_events_res* response,
                 ::google::protobuf::Closure* done)
        {
            typedef vtrc_example::stress_events_Stub stub_type;

            common::closure_holder holder(done);

            std::string payload(request->payload_size( ), '?');

            vtrc::unique_ptr<common::rpc_channel> channel;
            common::connection_iface_sptr c_sptr = c_->shared_from_this( );

            bool dis_wait = !request->wait( );

            if( request->callback( ) ) {
                channel.reset(unicast::create_callback_channel(
                                                        c_sptr, dis_wait ));
            } else {
                channel.reset(unicast::create_event_channel(
                                                        c_sptr, dis_wait ));
            }

            stub_wrapper_type stub(channel.get( ));
            vtrc_example::event_req req;
            req.set_payload( payload );

            req.set_is_event( dis_wait );

            for( unsigned i=0; i<request->count( ); ++i ) {
                req.set_id( i );
                stub.call_request( &stub_type::event, &req );
            }

        }

        void recursive_call(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::recursive_call_req* request,
                 ::vtrc_example::recursive_call_res* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);
            if( request->balance( ) == 0 ) return;

            vtrc::unique_ptr<common::rpc_channel> channel
                (unicast::create_callback_channel( c_->shared_from_this( ),
                                                   false ));

            stub_wrapper_type stub( channel.get( ) );
            stub.call_request( &stub_type::recursive_callback, request );
        }

        void shutdown(::google::protobuf::RpcController* controller,
                             const ::vtrc_example::shutdown_req* request,
                             ::vtrc_example::shutdown_res* response,
                             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);
            app_.stop( );
        }

    };

}

namespace stress {

    gpb::Service *create_service(common::connection_iface *c ,
                                 application &app)
    {
        return new stress_service_impl( app, c );
    }

    std::string service_name( )
    {
        return stress_service_impl::descriptor( )->full_name( );
    }

}
