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
    }

    struct rpc_channel_c::impl {

        vtrc::weak_ptr<common::connection_iface> connection_;

        rpc_channel *parent_;
        const unsigned mess_type_;
        const bool     disable_wait_;

        impl(vtrc::shared_ptr<common::connection_iface> c, bool insert, bool dw)
            :connection_(c)
            ,mess_type_(insert ? message_info::MESSAGE_CALL
                               : message_info::MESSAGE_INSERTION_CALL)
            ,disable_wait_(dw)
        {}

        static bool waitable_call( const lowlevel_unit_sptr &llu )
        {
            const bool  has = llu->opt( ).has_wait( );
            return !has || (has && llu->opt( ).wait( ));
        }

        static void configure_message( common::connection_iface_sptr c,
                                       unsigned mess_type,
                                       lowlevel_unit_sptr llu )
        {
            const common::call_context *cc(common::call_context::get(c));
            switch( mess_type ) {
            case message_info::MESSAGE_INSERTION_CALL:
                if( cc && cc->get_lowlevel_message( )->opt( ).wait( ) ) {
                    llu->mutable_info( )->set_message_type( mess_type );
                    llu->set_id( cc->get_lowlevel_message( )->id( ) );
                    break;
                } else {
                    mess_type = message_info::MESSAGE_CALL;
                }
            case message_info::MESSAGE_CALL:
                llu->mutable_info( )->set_message_type( mess_type );
                llu->set_id(c->get_protocol( ).next_index( ));
                break;
            }
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

            lowlevel_unit_sptr llu(
                        parent_->create_lowlevel(method, request, response));

            const vtrc_rpc_lowlevel::options &call_opt
                            ( clk->get_protocol( ).get_method_options(method) );

            if( disable_wait_ )
                llu->mutable_opt( )->set_wait( false );
            else
                llu->mutable_opt( )->set_wait( call_opt.wait( ) );

            configure_message( clk, mess_type_, llu );
            uint64_t call_id = llu->id( );

            rpc_channel::context_holder ch(&clk->get_protocol( ), llu.get( ));
            ch.ctx_->set_call_options( call_opt );

            llu->set_id( call_id );

            if( llu->opt( ).wait( ) ) { // WAITABLE CALL

                parent_->process_waitable_call( call_id, llu, response,
                                                clk, call_opt );

            } else { // NOT WAITABLE CALL
                clk->get_protocol( ).call_rpc_method( *llu );
            }
        }
    };

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c )
        :impl_(new impl(c, false, false))
    {
        impl_->parent_ = this;
    }

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c, bool dw )
        :impl_(new impl(c, dw, false))
    {
        impl_->parent_ = this;
    }

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c,
                                  bool disable_wait, bool ins)
        :impl_(new impl(c, disable_wait, ins))
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
