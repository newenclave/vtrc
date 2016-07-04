#include <algorithm>
#include <map>
#include <iostream>

#include "google/protobuf/descriptor.h"

#include "vtrc-rpc-lowlevel.pb.h"
#include "vtrc-rpc-options.pb.h"

#include "vtrc-rpc-channel.h"
#include "vtrc-protocol-layer.h"
#include "vtrc-exception.h"

#include "vtrc-bind.h"

namespace vtrc { namespace common  {

    namespace gpb = google::protobuf;
    namespace ph  = vtrc::placeholders;

    namespace {

        void default_error_cb( unsigned code, unsigned cat, const char *mess )
        {
            vtrc::common::exception e( code, cat, mess );
            vtrc::common::raise( e );
        }

        void default_chan_error_cb( const char *mess )
        {
            vtrc::common::exception e( rpc::errors::ERR_CHANNEL, mess );
            vtrc::common::raise( e );
        }

        rpc_channel::proto_error_cb_type get_default_error_cb( )
        {
            return vtrc::bind( default_error_cb, ph::_1, ph::_2, ph::_3 );
        }

        rpc_channel::channel_error_cb_type get_default_chan_error_cb( )
        {
            return vtrc::bind( default_chan_error_cb, ph::_1 );
        }

        bool can_accept_callbacks( const common::call_context *cc )
        {

#if 1   /// yeap. we always set it up

            rpc::lowlevel_unit const *llu  = cc->get_lowlevel_message( );
            return llu->opt( ).wait( ) && llu->opt( ).accept_callbacks( );

#else   /// is not compatible with protobuf3

            rpc::options       const *opts = cc->get_call_options( );
            rpc::lowlevel_unit const *llu  = cc->get_lowlevel_message( );

            bool accept_callbacks = llu->opt( ).has_accept_callbacks( )
                                  ? llu->opt( ).accept_callbacks( )
                                  : opts->accept_callbacks( );

            bool wait = llu->opt( ).has_wait( )
                      ? llu->opt( ).wait( )
                      : opts->wait( );

            return  wait && accept_callbacks;
#endif
        }

    }

    struct rpc_channel::impl {

        unsigned direct_type_;
        unsigned callback_type_;
        unsigned flags_;

        vtrc::uint64_t static_target_;
        bool           accept_callbacks_;

        std::string data_;

        rpc_channel::proto_error_cb_type    error_cb_;
        rpc_channel::channel_error_cb_type  chan_error_cb_;

        impl( unsigned direct_call_type, unsigned callback_type )
            :direct_type_(direct_call_type)
            ,callback_type_(callback_type)
            ,flags_(rpc_channel::DEFAULT)
            ,static_target_(0)
            ,accept_callbacks_(false)
            ,error_cb_(get_default_error_cb( ))
            ,chan_error_cb_(get_default_chan_error_cb( ))
        {
            const common::call_context *ctx = common::call_context::get( );
            if( ctx ) {
                accept_callbacks_ = can_accept_callbacks( ctx );
                static_target_    = ctx->get_lowlevel_message( )->id( );
            }
        }
    };

    rpc_channel::rpc_channel(unsigned direct_call_type, unsigned callback_type)
        :impl_(new impl(direct_call_type, callback_type))
    { }

    rpc_channel::~rpc_channel( )
    {
        delete impl_;
    }

    rpc_channel::lowlevel_unit_sptr rpc_channel::create_lowlevel(
                                        const gpb::MethodDescriptor *method,
                                        const gpb::Message */*request*/,
                                              gpb::Message *response)
    {
        lowlevel_unit_sptr llu(vtrc::make_shared<lowlevel_unit_type>( ));

        const std::string &serv_name(method->service( )->full_name( ));
        const std::string &meth_name(method->name( ));

        llu->mutable_call( )->set_service_id( serv_name );
        llu->mutable_call( )->set_method_id( meth_name );

//        if( request ) {
//            llu->set_request( request->SerializeAsString( ) );
//        }

        llu->mutable_opt( )->set_accept_response( response != NULL );

        return llu;
    }

    rpc_channel::lowlevel_unit_sptr rpc_channel::make_lowlevel(
                        const google::protobuf::MethodDescriptor* method,
                        const google::protobuf::Message* request,
                              google::protobuf::Message* response )
    {
//        rpc_channel::lowlevel_unit_sptr res =
//                create_lowlevel( method, request, response );
//        return res;
        return create_lowlevel( method, request, response );
    }

    rpc_channel::lowlevel_unit_sptr rpc_channel::raw_call(
            rpc_channel::lowlevel_unit_sptr /*llu*/,
            const google::protobuf::Message * /*request*/,
            lowlevel_closure_type /*callbacks*/ )
    {
        ;;; /// nothing to do here
        return rpc_channel::lowlevel_unit_sptr( );
    }

    void rpc_channel::set_channel_data( const std::string &data )
    {
        impl_->data_ = data;
    }

    void rpc_channel::set_flags( unsigned flags )
    {
        impl_->flags_ = flags;
    }

    unsigned rpc_channel::get_flags( ) const
    {
        return impl_->flags_;
    }

    void rpc_channel::set_flag( unsigned value )
    {
        impl_->flags_ |= value;
    }

    void rpc_channel::reset_flag( unsigned value )
    {
        impl_->flags_ &= (~value);
    }

    const std::string &rpc_channel::channel_data( )
    {
        return impl_->data_;
    }

    void rpc_channel::configure_message( common::connection_iface_sptr c,
                                         unsigned mess_type,
                                         const gpb::Message *req,
                                         lowlevel_unit_type &llu ) const
    {
        const common::call_context *cc(common::call_context::get( ));

        llu.set_id(c->get_protocol( ).next_index( ));

        if( req ) {
            llu.set_request( c->get_protocol( ).get_lowlevel( )
                              ->serialize_message( req ) );
        }

        if( mess_type == impl_->callback_type_ ) {

            if( get_flags( ) & rpc_channel::USE_STATIC_CONTEXT ) {

                if( !impl_->accept_callbacks_ ) {
                    impl_->chan_error_cb_( "Invalid context." );
                    return;
                }

                llu.mutable_info( )->set_message_type( mess_type );
                llu.set_target_id( impl_->static_target_ );

            } else if( cc && can_accept_callbacks( cc ) ) {

                llu.mutable_info( )->set_message_type( mess_type );
                llu.set_target_id( cc->get_lowlevel_message( )->id( ) );

            } else {

                llu.mutable_info( )->set_message_type( impl_->direct_type_ );

            }
        } else {
            llu.mutable_info( )->set_message_type( mess_type );
        }
    }

    rpc_channel::lowlevel_unit_sptr rpc_channel::call_and_wait_raw (
                        google::protobuf::uint64 call_id,
                        lowlevel_unit_type &llu,
                        common::connection_iface_sptr &cl,
                        lowlevel_closure_type events,
                        const rpc::options *call_opt ) const
    {
        if( !cl->active( ) ) {
            get_channel_error_callback( )( "Channel is not ready." );
            return false;
        }

        struct slot_erase {

            common::protocol_iface &proto_;
            google::protobuf::uint64 id_;

            slot_erase( common::protocol_iface &proto, gpb::uint64 id )
                :proto_(proto)
                ,id_(id)
            { }

            ~slot_erase( )
            {
                proto_.erase_slot( id_ );
            }
        };

        slot_erase slot_keeper(cl->get_protocol( ), call_id);

        cl->get_protocol( ).call_rpc_method( call_id, llu );

        const unsigned mess_type( llu.info( ).message_type( ) );

        //lowlevel_unit_sptr top (vtrc::make_shared<lowlevel_unit_type>( ));
        lowlevel_unit_sptr top;
        bool wait = true;

        if( !events ) {
            llu.mutable_opt( )->set_accept_callbacks( false );
        }

        while( wait ) {

            //lowlevel_unit_sptr top (vtrc::make_shared<lowlevel_unit_type>( ));
            //top->Clear( );

            top = vtrc::make_shared<lowlevel_unit_type>( );

            cl->get_protocol( ).read_slot_for( call_id, top,
                                               call_opt->timeout( ) );

            if( top->error( ).code( ) != rpc::errors::ERR_NO_ERROR ) {
                wait = false;
            } else if( top->info( ).message_type( ) != mess_type ) {
                if( events ) {
                    events( *top );
                }
            } else {
                wait = false;
            }
        }
        //cl->get_protocol( ).erase_slot( call_id );
        return top;
    }

    const rpc_channel::proto_error_cb_type &
          rpc_channel::get_proto_error_callback( ) const
    {
        return impl_->error_cb_;
    }

    void rpc_channel::set_proto_error_callback(
            const rpc_channel::proto_error_cb_type &value )
    {
        if( value ) {
            impl_->error_cb_ = value;
        } else {
            impl_->error_cb_ = get_default_error_cb( );
        }
    }

    const rpc_channel::channel_error_cb_type &
          rpc_channel::get_channel_error_callback( ) const
    {
        return impl_->chan_error_cb_;
    }

    void rpc_channel::set_channel_error_callback(
            const rpc_channel::channel_error_cb_type &value )
    {
        if( value ) {
            impl_->chan_error_cb_ = value;
        } else {
            impl_->chan_error_cb_ = get_default_chan_error_cb( );
        }
    }

    void rpc_channel::set_static_context( const common::call_context *ctx )
    {
        if( ctx ) {
            impl_->accept_callbacks_ = can_accept_callbacks( ctx );
            impl_->static_target_ = ctx->get_lowlevel_message( )->id( );
        } else {
            impl_->accept_callbacks_ = false;
            impl_->static_target_    = 0;
        }
    }

    bool rpc_channel::call_and_wait( google::protobuf::uint64 call_id,
                                     const lowlevel_unit_type &llu,
                                     google::protobuf::RpcController */*ctrl*/,
                                     google::protobuf::Message *response,
                                     connection_iface_sptr &cl,
                                     const rpc::options *call_opt ) const
    {
        if( !cl->active( ) ) {
            get_channel_error_callback( )( "Channel is not ready." );
            return false;
        }

        struct slot_erase {

            common::protocol_iface &proto_;
            google::protobuf::uint64 id_;

            slot_erase( common::protocol_iface &proto, gpb::uint64 id )
                :proto_(proto)
                ,id_(id)
            { }

            ~slot_erase( )
            {
                proto_.erase_slot( id_ );
            }
        };

        slot_erase slot_keeper(cl->get_protocol( ), call_id);
        cl->get_protocol( ).call_rpc_method( call_id, llu );

        const unsigned mess_type(llu.info( ).message_type( ));

        bool wait = true;

        //lowlevel_unit_sptr top (vtrc::make_shared<lowlevel_unit_type>( ));

        while( wait ) {

            lowlevel_unit_sptr top (vtrc::make_shared<lowlevel_unit_type>( ));
            //top->Clear( );

            cl->get_protocol( ).read_slot_for( call_id, top,
                                               call_opt->timeout( ) );

            if( top->error( ).code( ) != rpc::errors::ERR_NO_ERROR ) {
                //cl->get_protocol( ).erase_slot( call_id );
                impl_->error_cb_( top->error( ).code( ),
                                  top->error( ).category( ),
                                  top->error( ).additional( ).c_str( ) );
                return false;
            }

            /// from client: call, insertion_call
            /// from server: event, callback
            if( top->info( ).message_type( ) != mess_type ) {
                cl->get_protocol( ).make_local_call( top );
            } else {
                if( response ) {
                    cl->get_protocol( )
                       .get_lowlevel( )
                      ->parse_message( top->response( ), response );
                    //response->ParseFromString( top->response( ) );
                }
                wait = false;
            }
        }
        //cl->get_protocol( ).erase_slot( call_id );
        return true;
    }

    protocol_iface &rpc_channel::get_protocol(connection_iface &cl)
    {
        return cl.get_protocol( );
    }

    void rpc_channel::CallMethod( const gpb::MethodDescriptor *method,
                                        gpb::RpcController *controller,
                                  const gpb::Message *request,
                                        gpb::Message *response,
                                        gpb::Closure *done)
    {
        lowlevel_unit_sptr llu( create_lowlevel( method, request, response ) );
        if( !impl_->data_.empty( ) ) {
            llu->set_channel_data( impl_->data_ );
        }
        send_message( *llu, method, controller, request, response, done );
    }

    void rpc_channel::call_rpc_method( common::connection_iface *c,
                          const lowlevel_unit_type &llu )
    {
        c->get_protocol( ).call_rpc_method( llu );
    }

    void rpc_channel::call_rpc_method( common::connection_iface *c,
                                       vtrc::uint64_t slot_id,
                                       const lowlevel_unit_type &llu )
    {
        c->get_protocol( ).call_rpc_method( slot_id, llu );
    }

    void rpc_channel::read_slot_for( common::connection_iface *c,
                                     vtrc::uint64_t slot_id,
                                     lowlevel_unit_sptr &mess,
                                     vtrc::uint64_t microsec )
    {
        c->get_protocol( ).read_slot_for( slot_id, mess, microsec );
    }

    void rpc_channel::erase_slot( common::connection_iface *c,
                                  vtrc::uint64_t slot_id )
    {
        c->get_protocol( ).erase_slot( slot_id );
    }

    void rpc_channel::make_local_call( common::connection_iface *c,
                                       lowlevel_unit_sptr llu )
    {
        c->get_protocol( ).make_local_call( llu );
    }


}}

