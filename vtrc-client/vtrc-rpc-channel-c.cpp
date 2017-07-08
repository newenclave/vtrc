#include <algorithm>
#include <iostream>
#include "vtrc/client/rpc-channel-c.h"

#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"

#include "vtrc-rpc-lowlevel.pb.h"
#include "vtrc-rpc-options.pb.h"

#include "vtrc/common/connection-iface.h"
#include "vtrc/common/call-context.h"
#include "vtrc/common/exception.h"
#include "vtrc/common/closure-holder.h"

#include "vtrc-common/vtrc-protocol-layer.h"
#include "vtrc-chrono.h"

namespace vtrc { namespace client {

    namespace gpb = google::protobuf;

    namespace {

        typedef vtrc::shared_ptr<rpc::lowlevel_unit> lowlevel_unit_sptr;
        typedef rpc::message_info message_info;

        const unsigned direct_call_type = message_info::MESSAGE_CLIENT_CALL;
        const unsigned callback_type    = message_info::MESSAGE_CLIENT_CALLBACK;

        bool can_accept_callbacks( const common::call_context *cc )
        {
            rpc::lowlevel_unit const *llu  = cc->get_lowlevel_message( );
            return llu->opt( ).wait( ) && llu->opt( ).accept_callbacks( );
        }

        typedef common::protocol_layer::context_holder context_holder;
    }

    struct rpc_channel_c::impl {

        vtrc::weak_ptr<common::connection_iface> connection_;

        rpc_channel_c *parent_;
        bool           accept_callbacks_;
        vtrc::uint64_t target_id_;

        impl(vtrc::shared_ptr<common::connection_iface> c)
            :connection_(c)
            ,parent_(NULL)
            ,accept_callbacks_(false)
            ,target_id_(0)
        {
            const common::call_context *cc(common::call_context::get( ));
            if( cc && can_accept_callbacks( cc ) ) {
                accept_callbacks_ = true;
                target_id_ = cc->get_lowlevel_message( )->id( );
            }
        }

        unsigned flags( ) const
        {
            return parent_->get_flags( );
        }

        bool disable_wait( ) const
        {
            return (flags( ) & common::rpc_channel::DISABLE_WAIT);
        }

        bool static_context( ) const
        {
            return (flags( ) & common::rpc_channel::USE_STATIC_CONTEXT);
        }

        unsigned message_type( ) const
        {
            return ( flags( ) & common::rpc_channel::USE_CONTEXT_CALL )
                   ? message_info::MESSAGE_CLIENT_CALLBACK
                   : message_info::MESSAGE_CLIENT_CALL;
        }

        bool alive( ) const
        {
            return connection_.lock( ) != NULL;
        }

        common::protocol_iface &get_protocol(common::connection_iface_sptr &clk)
        {
            return parent_->get_protocol( *clk );
        }

        void send_message( lowlevel_unit_type &llu,
                     const gpb::MethodDescriptor *method,
                           gpb::RpcController    *controller,
                     const gpb::Message          *request,
                           gpb::Message          *response,
                           gpb::Closure          *done   )
        {
            common::closure_holder done_holder(done);
            common::connection_iface_sptr clk(connection_.lock( ));

            if( clk.get( ) == NULL ) {
                if( controller ) {
                    controller->SetFailed( "Connection lost" );
                }
                parent_->get_channel_error_callback( )( "Connection lost" );
                return;
            }

            const rpc::options *call_opt
                            ( get_protocol( clk ).get_method_options(method) );

            if( disable_wait( ) ) {
                llu.mutable_opt( )->set_wait( false );
            } else {
                llu.mutable_opt( )->set_wait( call_opt->wait( ) );
                llu.mutable_opt( )->set_accept_callbacks
                                            ( call_opt->accept_callbacks( ) );
            }

            parent_->configure_message_for( clk, request, llu );
            vtrc::uint64_t call_id = llu.id( );

            if( llu.opt( ).wait( ) ) {  /// Send and wait

                context_holder ch( &llu );
                ch.ctx_->set_call_options( call_opt );
                ch.ctx_->set_done_closure( done );

                parent_->call_and_wait( call_id, llu, controller,
                                        response, clk, call_opt );

            } else {                    /// Send and ... just send
                parent_->call_rpc_method( clk.get( ), llu );
            }

        }

        lowlevel_unit_sptr send_raw( lowlevel_unit_sptr &llu,
                                     const gpb::Message* request,
                                     common::lowlevel_closure_type cbacks )
        {
            common::connection_iface_sptr clk(connection_.lock( ));

            if( clk.get( ) == NULL ) {
                parent_->get_channel_error_callback( )( "Connection lost" );
                return lowlevel_unit_sptr( );
            }

            parent_->configure_message_for( clk, request, *llu );
            vtrc::uint64_t call_id = llu->id( );

            rpc::options call_opt;
            lowlevel_unit_sptr res;

            if( llu->opt( ).wait( ) ) {  /// Send and wait

                context_holder ch( llu.get( ) );
                ch.ctx_->set_call_options( &call_opt );

                res = parent_->call_and_wait_raw( call_id, *llu,
                                                  clk, cbacks, &call_opt );

            } else {                    /// Send and ... just send
                parent_->call_rpc_method( clk.get( ), *llu );
            }
            return res;
        }

        rpc_channel::lowlevel_unit_sptr make_lowlevel(
                            const gpb::MethodDescriptor* method,
                            const gpb::Message* request,
                                  gpb::Message* response ) const
        {
            rpc_channel::lowlevel_unit_sptr res =
                    create_lowlevel( method, request, response );

            common::connection_iface_sptr clk(connection_.lock( ));

            if( clk.get( ) == NULL ) {
                parent_->get_channel_error_callback( )( "Connection lost" );
                return rpc_channel::lowlevel_unit_sptr( );
            }

            parent_->configure_message_for( clk, request, *res );
            return res;
        }

    };

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c )
        :common::rpc_channel(direct_call_type, callback_type)
        ,impl_(new impl(c))
    {
        impl_->parent_ = this;
        set_flags( common::rpc_channel::DEFAULT );
    }

    rpc_channel_c::rpc_channel_c( common::connection_iface_sptr c,
                                  unsigned opts )
        :common::rpc_channel(direct_call_type, callback_type)
        ,impl_(new impl(c))
    {
        impl_->parent_ = this;
        set_flags( opts );
    }

    rpc_channel_c::~rpc_channel_c( )
    {
        delete impl_;
    }

    bool rpc_channel_c::alive( ) const
    {
        return impl_->alive( );
    }

    lowlevel_unit_sptr rpc_channel_c::raw_call( lowlevel_unit_sptr llu,
                                    const gpb::Message* request,
                                    common::lowlevel_closure_type callbacks )
    {
        return impl_->send_raw( llu, request, callbacks );
    }

    void rpc_channel_c::configure_message_for( common::connection_iface_sptr c,
                                    const gpb::Message* request,
                                    rpc_channel::lowlevel_unit_type &llu) const
    {
        configure_message( c, impl_->message_type( ), request, llu );
    }

    rpc_channel_c::lowlevel_unit_sptr rpc_channel_c::make_lowlevel(
                        const gpb::MethodDescriptor* method,
                        const gpb::Message* request,
                              gpb::Message* response )
    {
        return impl_->make_lowlevel( method, request, response );
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

    namespace channels {
        common::rpc_channel *create( common::connection_iface_sptr c )
        {
            return new rpc_channel_c( c );
        }
        common::rpc_channel *create( common::connection_iface_sptr c,
                                 unsigned flags )
        {
            return new rpc_channel_c( c, flags );
        }
    }

}}
