#include <iostream>

#include "stress-service-impl.h"
#include "protocol/stress.pb.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-server/vtrc-channels.h"
#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-stub-wrapper.h"

#include "vtrc-bind.h"
#include "vtrc-memory.h"

#include "google/protobuf/descriptor.h"

#include "stress-application.h"

namespace gpb = google::protobuf;
using namespace vtrc;

namespace  {

    typedef vtrc_example::stress_events_Stub    stub_type;
    typedef common::stub_wrapper<stub_type>     stub_wrapper_type;
    typedef vtrc::shared_ptr<stub_wrapper_type> stub_wrapper_sptr;
    typedef vtrc::weak_ptr<stub_wrapper_type>   stub_wrapper_wptr;

    using namespace server::channels;
    using server::channels::unicast::create_event_channel;
    using server::channels::unicast::create_callback_channel;

    stub_wrapper_type *create_event( common::connection_iface *c )
    {
        return new stub_wrapper_type(
                    create_event_channel( c->shared_from_this( ) ),
                    true );
    }

    common::rpc_channel *create_callback( common::connection_iface *c )
    {
        return create_callback_channel( c->shared_from_this( ) );
    }

    class stress_service_impl: public vtrc_example::stress_service {

        stress::application      &app_;
        common::connection_iface *c_;
        stub_wrapper_sptr         event_channel_;
        vtrc::unique_ptr<common::rpc_channel> channel_;
    public:

        stress_service_impl( stress::application &app,
                             common::connection_iface *c )
            :app_(app)
            ,c_(c)
            ,event_channel_(create_event(c_))
            ,channel_(create_callback(c_))
        {
            channel_->set_flag( vtrc::common::rpc_channel::USE_STATIC_CONTEXT );
        }

    private:

        bool timer_cb( unsigned id, unsigned error_code,
                       common::connection_iface_wptr client,
                       stub_wrapper_wptr channel )
        {
            common::connection_iface_sptr lck( client.lock( ) );
            if( !lck ) {
                return false;
            }

            stub_wrapper_sptr eventor( channel.lock( ) );
            if( !eventor ) {
                return false;
            }

            std::cout << "Tick timer: " << id << " " << error_code << "\n";

            vtrc_example::timer_register_res req;
            req.set_id( id );
            eventor->call_request( &stub_type::timer_event, &req );
            return true;
        }

        void reg(::google::protobuf::RpcController* controller,
                     const ::vtrc_example::timer_register_req* request,
                     ::vtrc_example::timer_register_res* response,
                     ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);

            stress::application::timer_callback cb(
                        vtrc::bind( &stress_service_impl::timer_cb, this,
                                    vtrc::placeholders::_1,
                                    vtrc::placeholders::_2,
                                    c_->weak_from_this( ),
                                    stub_wrapper_wptr(event_channel_)));

            response->set_id( app_.add_timer_event(
                                  request->microseconds( ), cb ) );

        }

        void unreg(::google::protobuf::RpcController* controller,
                     const ::vtrc_example::timer_register_res* request,
                     ::vtrc_example::empty* response,
                     ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);
        }

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

            common::connection_iface_sptr c_sptr = c_->shared_from_this( );

            bool dis_wait = !request->wait( );

            vtrc::unique_ptr<common::rpc_channel> channel;

            if( request->callback( ) ) {
                std::cout << "Callbacks\n";
                channel.reset(unicast::create_callback_channel(
                                                       c_sptr, dis_wait ));
            } else {
                std::cout << "Events\n";
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
            std::cout << "Sent " << request->count( ) << " messages\n";

        }

        void recursive_call(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::recursive_call_req* request,
                 ::vtrc_example::recursive_call_res* response,
                 ::google::protobuf::Closure* done)
        {
//            vtrc::shared_ptr<common::closure_holder>
//                    holder(new common::closure_holder(done));

//            if( request->balance( ) == 2 ) {
//                vtrc::server::application *app = &app_.get_application( );
//                app->get_rpc_service( ).post( [done]( ) {
//                    sleep( 1 );
//                    done->Run( );
//                    std::cout << "reset holder " << std::hex << done;
//                } );
//                return;
//            }

            if( request->balance( ) == 0 ) {
                done->Run( );
                return;
            };

//            channel_->set_static_context( NULL ); /// thows channel error!
            channel_->set_static_context( vtrc::common::call_context::get( ) );

//            vtrc::unique_ptr<common::rpc_channel> channel
//                (unicast::create_callback_channel( c_->shared_from_this( ),
//                                                   false ));

            stub_wrapper_type stub( channel_.get( ) );
            stub.call_request( &stub_type::recursive_callback, request );
            done->Run( );
        }

        void shutdown(::google::protobuf::RpcController* controller,
                             const ::vtrc_example::shutdown_req* request,
                             ::vtrc_example::shutdown_res* response,
                             ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);
            app_.stop( );
        }

        void close_me( ::google::protobuf::RpcController* /*controller*/,
                       const ::vtrc_example::empty*       /*request*/,
                       ::vtrc_example::empty*             /*response*/,
                       ::google::protobuf::Closure* done)
        {
            common::closure_holder holder(done);
            std::cout << "close request from: " << c_->name( ) << std::endl;
            c_->close( );
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
