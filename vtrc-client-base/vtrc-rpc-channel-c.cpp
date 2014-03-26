#include "vtrc-rpc-channel-c.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-protocol-layer.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include "vtrc-chrono.h"

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
            const bool  has = llu->opt( ).has_wait( );
            return !has || (has && llu->opt( ).wait( ));
        }

        void CallMethod(const gpb::MethodDescriptor* method,
                        gpb::RpcController* controller,
                        const gpb::Message* request,
                        gpb::Message* response,
                        gpb::Closure* done)
        {
            //common::closure_holder hold(done);

            common::connection_iface_sptr clk(connection_.lock( ));

            if( clk.get( ) == NULL ) {
                throw vtrc::common::exception( vtrc_errors::ERR_CHANNEL,
                                               "Connection lost");
            }

            const vtrc_rpc_lowlevel::options &call_opt
                             ( clk->get_protocol( ).get_method_options(method) );

            lowlevel_unit_sptr llu(
                        parent_->create_lowlevel(method, request, response));

            llu->mutable_info( )->set_message_type(
                               vtrc_rpc_lowlevel::message_info::MESSAGE_CALL );

            uint64_t call_id = clk->get_protocol( ).next_index( );

            llu->set_id( call_id );

            if( call_opt.wait( ) && llu->opt( ).wait( ) ) { // WAITABLE CALL

                parent_->process_waitable_call( call_id, llu, response,
                                                clk, call_opt );

            } else { // NOT WAITABLE CALL
                clk->get_protocol( ).call_rpc_method( *llu );
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
