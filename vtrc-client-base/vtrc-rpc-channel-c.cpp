#include "vtrc-rpc-channel-c.h"

#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"

#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "protocol/vtrc-rpc-options.pb.h"

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
            vtrc_rpc::lowlevel_unit
        > lowlevel_unit_sptr;
        typedef vtrc_rpc::message_info message_info;

        const unsigned direct_call_type = message_info::MESSAGE_CALL;
        const unsigned callback_type    = message_info::MESSAGE_INSERTION_CALL;

    }

    struct rpc_channel_c::impl {

        vtrc::weak_ptr<common::connection_iface> connection_;

        rpc_channel_c *parent_;
        const unsigned mess_type_;
        const bool     disable_wait_;

        impl(vtrc::shared_ptr<common::connection_iface> c,
                              common::rpc_channel::options opts)
            :connection_(c)
            ,mess_type_(select_message_type(opts))
            ,disable_wait_(select_message_wait(opts))
        { }

        unsigned select_message_type (common::rpc_channel::options opts) const
        {
            return (opts & common::rpc_channel::USE_CONTEXT_CALL)
                   ? message_info::MESSAGE_INSERTION_CALL
                   : message_info::MESSAGE_CALL;
        }

        bool select_message_wait (common::rpc_channel::options opts) const
        {
            return (opts & common::rpc_channel::DISABLE_WAIT);
        }

        common::protocol_layer &get_protocol(common::connection_iface_sptr &clk)
        {
            return parent_->get_protocol( *clk );
        }

        void send_message( lowlevel_unit_type &llu,
                     const gpb::MethodDescriptor *method,
                           gpb::RpcController *controller,
                     const gpb::Message *  /*  request  */,
                           gpb::Message *      response,
                           gpb::Closure *      done   )
        {
            common::closure_holder done_holder(done);
            common::connection_iface_sptr clk(connection_.lock( ));

            if( clk.get( ) == NULL ) {
                if( controller )
                    controller->SetFailed( "Connection lost" );
                throw vtrc::common::exception( vtrc_errors::ERR_CHANNEL,
                                               "Connection lost");
            }

            const vtrc_rpc::options &call_opt
                            ( get_protocol( clk ).get_method_options(method) );

            if( disable_wait_ )
                llu.mutable_opt( )->set_wait( false );
            else {
                llu.mutable_opt( )->set_wait( call_opt.wait( ) );
                llu.mutable_opt( )->set_accept_callbacks
                                               ( call_opt.accept_callbacks( ) );
            }

            parent_->configure_message_for( clk, llu );
            uint64_t call_id = llu.id( );

            if( llu.opt( ).wait( ) ) {  /// WAITABLE CALL

                rpc_channel::context_holder ch( &get_protocol( clk ), &llu );
                ch.ctx_->set_call_options( call_opt );
                ch.ctx_->set_done_closure( done );

                parent_->process_waitable_call( call_id, llu, response,
                                                clk, call_opt );

            } else {                    /// NOT WAITABLE CALL
                get_protocol( clk ).call_rpc_method( llu );
            }

        }

    };

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c )
        :common::rpc_channel(direct_call_type, callback_type)
        ,impl_(new impl(c, common::rpc_channel::DEFAULT))
    {
        impl_->parent_ = this;
    }

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c,
                                  common::rpc_channel::options opts )
        :common::rpc_channel(direct_call_type, callback_type)
        ,impl_(new impl(c, opts))
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
