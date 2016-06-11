#include <algorithm>

#include "vtrc-channels.h"
#include "vtrc-protocol-layer-s.h"

#include "vtrc-common/vtrc-call-context.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-common/vtrc-protocol-layer.h"

#include "vtrc-errors.pb.h"
#include "vtrc-rpc-lowlevel.pb.h"
#include "vtrc-rpc-options.pb.h"

//#include "vtrc-chrono.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

namespace vtrc { namespace server {

    namespace {

        namespace gpb = google::protobuf;

        typedef rpc::message_info message_info;

        const unsigned direct_call_type = message_info::MESSAGE_SERVER_CALL;
        const unsigned callback_type    = message_info::MESSAGE_SERVER_CALLBACK;

        namespace gpb = google::protobuf;

        typedef rpc::lowlevel_unit                   lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        typedef common::protocol_layer::context_holder context_holder;

        class unicast_channel: public common::rpc_channel {

            common::connection_iface_wptr client_;

        public:

            unicast_channel( common::connection_iface_sptr c,
                             unsigned flags )
                :common::rpc_channel(direct_call_type, callback_type)
                ,client_(c)
            {
                set_flags( flags );
            }

            bool alive( ) const
            {
                return client_.lock( ) != NULL;
            }

            bool disable_wait( ) const
            {
                return (get_flags( ) & common::rpc_channel::DISABLE_WAIT);
            }

            unsigned message_type( ) const
            {
                return ( get_flags( ) & common::rpc_channel::USE_CONTEXT_CALL )
                       ? message_info::MESSAGE_SERVER_CALLBACK
                       : message_info::MESSAGE_SERVER_CALL;
            }

            lowlevel_unit_sptr raw_call( lowlevel_unit_sptr llu,
                                         const google::protobuf::Message* req,
                                         common::lowlevel_closure_type events )
            {
                common::connection_iface_sptr clnt (client_.lock( ));

                if( clnt.get( ) == NULL ) {
                    get_channel_error_callback( )( "Connection lost" );
                    return lowlevel_unit_sptr( );
                }

                const rpc::options call_opt;

                configure_message( clnt, message_type( ), req, *llu );
                const gpb::uint64 call_id = llu->id( );

                lowlevel_unit_sptr res;

                if( llu->opt( ).wait( ) ) { /// Send and wait

                    context_holder ch( llu.get( ) );
                    ch.ctx_->set_call_options( &call_opt );

                    res = call_and_wait_raw( call_id, *llu,
                                             clnt, events, &call_opt );

                } else {                  /// Send and ... just send
                    call_rpc_method( clnt.get( ), *llu );
                }
                return res;
            }

            lowlevel_unit_sptr make_lowlevel( const gpb::MethodDescriptor* met,
                                              const gpb::Message* req,
                                                    gpb::Message* res )
            {
                lowlevel_unit_sptr llu = create_lowlevel( met, req, res );

                common::connection_iface_sptr clnt (client_.lock( ));
                if( clnt.get( ) == NULL ) {
                    get_channel_error_callback( )( "Connection lost" );
                    return lowlevel_unit_sptr( );
                }

                const rpc::options *call_opt
                         ( get_protocol( *clnt ).get_method_options( met ) );


                if( disable_wait( ) ) {
                    llu->mutable_opt( )->set_wait(false);
                } else {
                    llu->mutable_opt( )->set_wait( call_opt->wait( ) );
                    llu->mutable_opt( )
                       ->set_accept_callbacks( call_opt->accept_callbacks( ) );
                }

                configure_message( clnt, message_type( ), req, *llu );

                return llu;
            }

            void send_message( lowlevel_unit_type &llu,
                               const gpb::MethodDescriptor* method,
                                     gpb::RpcController*    controller,
                               const gpb::Message*          request,
                                     gpb::Message*          response,
                                     gpb::Closure*          done )
            {
                common::closure_holder        done_holder(done);
                common::connection_iface_sptr clnt (client_.lock( ));

                if( clnt.get( ) == NULL ) {
                    if( controller ) {
                        controller->SetFailed( "Connection lost" );
                    }
                    get_channel_error_callback( )( "Connection lost" );
                    return;
                }

                const rpc::options *call_opt
                         ( get_protocol( *clnt ).get_method_options(method) );


                if( disable_wait( ) ) {
                    llu.mutable_opt( )->set_wait(false);
                } else {
                    llu.mutable_opt( )->set_wait(call_opt->wait( ));
                    llu.mutable_opt( )
                        ->set_accept_callbacks(call_opt->accept_callbacks( ));
                }

                configure_message( clnt, message_type( ), request, llu );
                const gpb::uint64 call_id = llu.id( );

                if( llu.opt( ).wait( ) ) { /// Send and wait

                    context_holder ch( &llu );
                    ch.ctx_->set_call_options( call_opt );
                    ch.ctx_->set_done_closure( done );

                    call_and_wait( call_id, llu, controller,
                                   response, clnt, call_opt );

                } else {                  /// Send and ... just send
                    call_rpc_method( clnt.get( ), llu );
                }
            }
        };

        class broadcast_channel: public common::rpc_channel {

            typedef broadcast_channel               this_type;
            vtrc::weak_ptr<common::connection_list> clients_;
            common::connection_iface_wptr           sender_;
            const unsigned                          message_type_;

        public:

            broadcast_channel( vtrc::weak_ptr<common::connection_list> clients,
                               unsigned mess_type)
                :common::rpc_channel(direct_call_type, callback_type)
                ,clients_(clients)
                ,message_type_(mess_type)
            { }

            broadcast_channel( vtrc::weak_ptr<common::connection_list> clients,
                               unsigned mess_type,
                               common::connection_iface_wptr   sender)
                :common::rpc_channel(direct_call_type, callback_type)
                ,clients_(clients)
                ,sender_(sender)
                ,message_type_(mess_type)
            { }

            bool alive( ) const
            {
                return clients_.lock( ) != NULL;
            }

            unsigned get_flags( ) const
            {
                return ( common::rpc_channel::DISABLE_WAIT )
                     | ( message_type_ == message_info::MESSAGE_SERVER_CALLBACK
                                        ? common::rpc_channel::USE_CONTEXT_CALL
                                        : 0 );
            }

            lowlevel_unit_sptr make_lowlevel(
                            const google::protobuf::MethodDescriptor* met,
                            const google::protobuf::Message* req,
                                  google::protobuf::Message* res )
            {
                lowlevel_unit_sptr llu = create_lowlevel( met, req, res );

                common::connection_iface_sptr clk(sender_.lock( ));
                vtrc::shared_ptr<common::connection_list>
                                                    lck_list(clients_.lock( ));
                if( !lck_list ) {
                    get_channel_error_callback( )( "Connection lost" );
                    return lowlevel_unit_sptr( );
                }

                llu->mutable_info( )->set_message_type( message_type_ );
                llu->mutable_opt( )->set_wait( false );

                return llu;
            }

            bool send_to_client( common::connection_iface_sptr  next,
                                 const common::connection_iface_sptr &sender,
                                 const google::protobuf::Message* request,
                                 lowlevel_unit_type &mess,
                                 unsigned mess_type)
            {
                if( sender != next && next->active( ) ) {
                    configure_message( next, mess_type, request, mess );
                    call_rpc_method( next.get( ), mess );
                }
                return true;
            }


            bool send_to_client2( common::connection_iface_sptr  next,
                                  const google::protobuf::Message* request,
                                  lowlevel_unit_type &mess,
                                  unsigned mess_type)
            {
                if( next->active( ) ) {
                    configure_message( next, mess_type, request, mess );
                    call_rpc_method( next.get( ), mess );
                }
                return true;
            }

            void send_message(lowlevel_unit_type &llu,
                    const google::protobuf::MethodDescriptor* /* method     */,
                          google::protobuf::RpcController*    /* controller */,
                    const google::protobuf::Message*          request,
                          google::protobuf::Message*          /* response   */,
                          google::protobuf::Closure* done )
            {
                common::closure_holder clhl(done);

                common::connection_iface_sptr clk(sender_.lock( ));
                vtrc::shared_ptr<common::connection_list>
                                                    lck_list(clients_.lock( ));
                if( !lck_list ) {
                    get_channel_error_callback( )( "Connection lost" );
                    return;
                }

                llu.mutable_info( )->set_message_type( message_type_ );
                llu.mutable_opt( )->set_wait( false );

//                const rpc_lowlevel::options &call_opt
//                          ( clk->get_protocol( ).get_method_options(method));

                if( clk ) {
                    lck_list->foreach_while(
                            vtrc::bind( &this_type::send_to_client, this,
                                         vtrc::placeholders::_1,
                                         vtrc::cref(clk),
                                         request,
                                         vtrc::ref(llu),
                                         message_type_) );
                } else {
                    lck_list->foreach_while(
                            vtrc::bind( &this_type::send_to_client2, this,
                                         vtrc::placeholders::_1,
                                         request,
                                         vtrc::ref(llu),
                                         message_type_) );
                }
            }
        };
    }

    namespace channels {

    namespace unicast {

        common::rpc_channel *create_event_channel(
                             common::connection_iface_sptr c, bool disable_wait)
        {
            const unsigned flags = disable_wait
                                 ? common::rpc_channel::DISABLE_WAIT
                                 : common::rpc_channel::DEFAULT;

            return new unicast_channel(c, flags);
        }

        common::rpc_channel *create_callback_channel(
                             common::connection_iface_sptr c, bool disable_wait)
        {
            const unsigned flags = common::rpc_channel::USE_CONTEXT_CALL
                                 | (disable_wait
                                 ? common::rpc_channel::DISABLE_WAIT
                                 : common::rpc_channel::DEFAULT);

            return new unicast_channel(c, flags);
        }

        common::rpc_channel *create(common::connection_iface_sptr c,
                                    unsigned flags )
        {
            return new unicast_channel(c, flags);
        }
    }

    namespace broadcast {

        common::rpc_channel *create_event_channel(
                                vtrc::shared_ptr<common::connection_list> cl,
                                common::connection_iface_sptr c )
        {
            static const unsigned message_type
                ( rpc::message_info::MESSAGE_SERVER_CALL );

            return new broadcast_channel(cl->weak_from_this( ),
                                         message_type, c->weak_from_this( ));

        }

        common::rpc_channel *create_event_channel(
                                vtrc::shared_ptr<common::connection_list> cl )
        {
            static const unsigned message_type
                ( rpc::message_info::MESSAGE_SERVER_CALL );
            return new broadcast_channel(cl->weak_from_this( ), message_type );
        }

        common::rpc_channel *create_callback_channel(
                               vtrc::shared_ptr<common::connection_list> cl,
                               common::connection_iface_sptr c )
        {
            static const unsigned message_type
                ( rpc::message_info::MESSAGE_SERVER_CALLBACK );
            return new broadcast_channel(cl->weak_from_this( ),
                                         message_type, c->weak_from_this( ));
        }

        common::rpc_channel *create_callback_channel(
                                 vtrc::shared_ptr<common::connection_list> cl)
        {
            static const unsigned message_type
                ( rpc::message_info::MESSAGE_SERVER_CALLBACK );
            return new broadcast_channel(cl->weak_from_this( ), message_type );
        }
    }

    }

}}

