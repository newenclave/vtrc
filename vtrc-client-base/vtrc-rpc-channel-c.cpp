#include "vtrc-rpc-channel-c.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-call-context.h"
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
        typedef vtrc_rpc_lowlevel::message_info message_info;

        const unsigned direct_call_type = message_info::MESSAGE_CALL;
        const unsigned callback_type    = message_info::MESSAGE_INSERTION_CALL;

    }

    struct rpc_channel_c::impl {

        vtrc::weak_ptr<common::connection_iface> connection_;

        rpc_channel_c *parent_;
        const unsigned mess_type_;
        const bool     disable_wait_;

        impl(vtrc::shared_ptr<common::connection_iface> c, bool dw, bool insert)
            :connection_(c)
            ,mess_type_(insert ? message_info::MESSAGE_INSERTION_CALL
                               : message_info::MESSAGE_CALL)
            ,disable_wait_(dw)
        {}

        void send_message( lowlevel_unit_type &llu,
                     const gpb::MethodDescriptor *method,
                           gpb::RpcController * /* controller*/,
                     const gpb::Message *       /*  request  */,
                           gpb::Message *response,
                           gpb::Closure *       /*    done   */ )
        {
            common::connection_iface_sptr clk(connection_.lock( ));

            if( clk.get( ) == NULL ) {
                throw vtrc::common::exception( vtrc_errors::ERR_CHANNEL,
                                               "Connection lost");
            }

            const vtrc_rpc_lowlevel::options &call_opt
                            ( clk->get_protocol( ).get_method_options(method) );

            if( disable_wait_ )
                llu.mutable_opt( )->set_wait( false );
            else
                llu.mutable_opt( )->set_wait( call_opt.wait( ) );

            parent_->configure_message_for( clk, llu );
            uint64_t call_id = llu.id( );

            rpc_channel::context_holder ch( &clk->get_protocol( ), &llu );
            ch.ctx_->set_call_options( call_opt );

            if( llu.opt( ).wait( ) ) { // WAITABLE CALL

                parent_->process_waitable_call( call_id, llu, response,
                                                clk, call_opt );

            } else { // NOT WAITABLE CALL
                clk->get_protocol( ).call_rpc_method( llu );
            }

        }

    };

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c )
        :common::rpc_channel(direct_call_type, callback_type)
        ,impl_(new impl(c, false, false))
    {
        impl_->parent_ = this;
    }

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c, bool dw )
        :common::rpc_channel(direct_call_type, callback_type)
        ,impl_(new impl(c, dw, false))
    {
        impl_->parent_ = this;
    }

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c,
                                  bool disable_wait, bool ins)
        :common::rpc_channel(direct_call_type, callback_type)
        ,impl_(new impl(c, disable_wait, ins))
    {
        impl_->parent_ = this;
    }

    rpc_channel_c::~rpc_channel_c( )
    {
        delete impl_;
    }

    void rpc_channel_c::configure_message_for(common::connection_iface_sptr c,
                            common::rpc_channel::lowlevel_unit_type &llu) const
    {
        configure_message( c, impl_->mess_type_, llu );
    }

    void rpc_channel_c::send_message( lowlevel_unit_type &llu,
                                const gpb::MethodDescriptor *method,
                                      gpb::RpcController *controller,
                                const gpb::Message *request,
                                      gpb::Message *response,
                                      gpb::Closure *done)
    {
        impl_->send_message( llu, method, controller, request, response, done );
    }

}}
