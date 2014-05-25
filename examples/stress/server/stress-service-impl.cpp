#include <iostream>

#include "stress-service-impl.h"
#include "protocol/stress.pb.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-server/vtrc-channels.h"

#include "google/protobuf/descriptor.h"

namespace gpb = google::protobuf;
using namespace vtrc;

namespace  {

    class stress_service_impl: public vtrc_example::stress_service {

        common::connection_iface *c_;

    public:

        stress_service_impl( common::connection_iface *c )
            :c_(c)
        { }

    private:

        void init(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::empty* request,
                 ::vtrc_example::empty* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);
        }


        void ping(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::ping_req* request,
                 ::vtrc_example::ping_res* response,
                 ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);
        }

        void generate_event(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::generate_events_req* request,
                 ::vtrc_example::generate_events_res* response,
                 ::google::protobuf::Closure* done)
        {

            using namespace server::channels;
            typedef vtrc_example::stress_events_Stub stub_type;

            common::closure_holder holder(done);

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

            stub_type stub( channel.get( ) );
            vtrc_example::event_req req;

            req.set_is_event( dis_wait );

            for( unsigned i=0; i<request->count( ); ++i ) {
                req.set_id( i );
                stub.event( NULL, &req, NULL, NULL );
            }

        }
    };

}

namespace stress {

    gpb::Service *create_service( common::connection_iface *c )
    {
        return new stress_service_impl( c );
    }

    std::string service_name( )
    {
        return stress_service_impl::descriptor( )->full_name( );
    }

}
