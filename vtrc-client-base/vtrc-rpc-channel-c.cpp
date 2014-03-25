#include "vtrc-rpc-channel-c.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-protocol-layer.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-closure-holder.h"

namespace vtrc { namespace client {

    namespace gpb = google::protobuf;

    namespace {
        typedef vtrc::shared_ptr <
            vtrc_rpc_lowlevel::lowlevel_unit
        > lowlevel_unit_sptr;
    }

    struct rpc_channel_c::impl {

        vtrc::weak_ptr<common::connection_iface> connection_;

        rpc_channel *parent_;

        impl(vtrc::shared_ptr<common::connection_iface> c)
            :connection_(c)
        {}

        static bool waitable_call( const lowlevel_unit_sptr &llu )
        {
            const bool  has = llu->info( ).has_wait_for_response( );
            return !has || (has && llu->info( ).wait_for_response( ));
        }

        void process_waitable_call( uint64_t call_id,
                                    lowlevel_unit_sptr &llu,
                                    gpb::Message* response,
                                    common::connection_iface_sptr &cl,
                                    const vtrc_rpc_lowlevel::options &call_opt)
        {
            cl->get_protocol( ).call_rpc_method( call_id, *llu );

            bool wait = true;

            while( wait ) {

                lowlevel_unit_sptr top(vtrc::make_shared<lowlevel_unit_type>());

                cl->get_protocol( ).read_slot_for( call_id, top,
                                                   call_opt.call_timeout( ) );

                if( top->error( ).code( ) != vtrc_errors::ERR_NO_ERROR ) {
                    cl->get_protocol( ).erase_slot( call_id );
                    throw vtrc::common::exception( top->error( ).code( ),
                                                 top->error( ).category( ),
                                                 top->error( ).additional( ) );
                }

                if( top->info( ).message_type( ) ==
                        vtrc_rpc_lowlevel::message_info::MESSAGE_CALLBACK )
                {
                    cl->get_protocol( ).make_call( top );
                } else {
                    response->ParseFromString( top->response( ) );
                    wait = false;
                }
            }

            cl->get_protocol( ).erase_slot( call_id );

        }

        void CallMethod(const gpb::MethodDescriptor* method,
                        gpb::RpcController* controller,
                        const gpb::Message* request,
                        gpb::Message* response,
                        gpb::Closure* done)
        {
            //common::closure_holder hold(done);

            common::connection_iface_sptr cl(connection_.lock( ));

            if( cl.get( ) == NULL ) {
                throw vtrc::common::exception( vtrc_errors::ERR_CHANNEL,
                                               "Connection lost");
            }

            const vtrc_rpc_lowlevel::options &call_opt
                             ( cl->get_protocol( ).get_method_options(method) );

            lowlevel_unit_sptr llu(
                        parent_->create_lowlevel(method, request, response));

            llu->mutable_info( )->set_message_type(
                               vtrc_rpc_lowlevel::message_info::MESSAGE_CALL );

            uint64_t call_id = cl->get_protocol( ).next_index( );

            llu->set_id( call_id );

            if( call_opt.wait( ) && waitable_call( llu ) ) { // WAITABLE CALL

                process_waitable_call( call_id, llu, response,
                                       cl, call_opt );

            } else { // NOT WAITABLE CALL
                cl->get_protocol( ).call_rpc_method( *llu );
            }
        }
    };

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c )
        :impl_(new impl(c))
    {
        impl_->parent_ = this;
    }

    rpc_channel_c::~rpc_channel_c( )
    {
        delete impl_;
    }

    void rpc_channel_c::CallMethod(const gpb::MethodDescriptor* method,
                    gpb::RpcController* controller,
                    const gpb::Message* request,
                    gpb::Message* response,
                    gpb::Closure* done)
    {
        impl_->CallMethod( method, controller, request, response, done );
    }

}}
