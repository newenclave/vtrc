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
            const unsigned                message_type_;
            const bool                    disable_wait_;

        public:

            unicast_channel( common::connection_iface_sptr c,
                             unsigned mess_type, bool disable_wait)
                :common::rpc_channel(direct_call_type, callback_type)
                ,client_(c)
                ,message_type_(mess_type)
                ,disable_wait_(disable_wait)
            { }

            bool alive( ) const
            {
                return client_.lock( ) != NULL;
            }

            lowlevel_unit_sptr raw_call( lowlevel_unit_sptr llu,
                                         common::lowlevel_closure_type events )
            {
                common::connection_iface_sptr clnt (client_.lock( ));

                if( clnt.get( ) == NULL ) {
                    get_channel_error_callback( )( "Connection lost" );
                    return lowlevel_unit_sptr( );
//                    throw vtrc::common::exception( rpc::errors::ERR_CHANNEL,
//                                                   "Connection lost");
                }

                const rpc::options call_opt;

                configure_message( clnt, message_type_, *llu );
                const gpb::uint64 call_id = llu->id( );

                lowlevel_unit_sptr res;

                if( llu->opt( ).wait( ) ) { /// Send and wait

                    context_holder ch( &get_protocol(*clnt), llu.get( ) );
                    ch.ctx_->set_call_options( &call_opt );

                    res = call_and_wait_raw( call_id, *llu,
                                             clnt, events, &call_opt );

                } else {                  /// Send and ... just send
                    get_protocol( *clnt ).call_rpc_method( *llu );
                }
                return res;
            }

            lowlevel_unit_sptr make_lowlevel(
                            const google::protobuf::MethodDescriptor* met,
                            const google::protobuf::Message* req,
                                  google::protobuf::Message* res )
            {
                lowlevel_unit_sptr llu = create_lowlevel( met, req, res );

                common::connection_iface_sptr clnt (client_.lock( ));
                if( clnt.get( ) == NULL ) {
                    get_channel_error_callback( )( "Connection lost" );
                    return lowlevel_unit_sptr( );
//                    throw vtrc::common::exception( rpc::errors::ERR_CHANNEL,
//                                                   "Connection lost");
                }

                const rpc::options *call_opt
                         ( get_protocol( *clnt ).get_method_options( met ) );


                if( disable_wait_ ) {
                    llu->mutable_opt( )->set_wait(false);
                } else {
                    llu->mutable_opt( )->set_wait( call_opt->wait( ) );
                    llu->mutable_opt( )
                       ->set_accept_callbacks( call_opt->accept_callbacks( ) );
                }

                configure_message( clnt, message_type_, *llu );

                return llu;
            }

            void send_message(lowlevel_unit_type &llu,
                        const google::protobuf::MethodDescriptor* method,
                              google::protobuf::RpcController*  controller,
                        const google::protobuf::Message*        /*request   */,
                              google::protobuf::Message*          response,
                              google::protobuf::Closure*          done )
            {
                common::closure_holder        done_holder(done);
                common::connection_iface_sptr clnt (client_.lock( ));

                if( clnt.get( ) == NULL ) {
                    if( controller ) {
                        controller->SetFailed( "Connection lost" );
                    }
                    get_channel_error_callback( )( "Connection lost" );
//                    throw vtrc::common::exception( rpc::errors::ERR_CHANNEL,
//                                                   "Connection lost");
                }

                const rpc::options *call_opt
                         ( get_protocol( *clnt ).get_method_options(method) );


                if( disable_wait_ ) {
                    llu.mutable_opt( )->set_wait(false);
                } else {
                    llu.mutable_opt( )->set_wait(call_opt->wait( ));
                    llu.mutable_opt( )
                        ->set_accept_callbacks(call_opt->accept_callbacks( ));
                }

                configure_message( clnt, message_type_, llu );
                const gpb::uint64 call_id = llu.id( );

                if( llu.opt( ).wait( ) ) { /// Send and wait

                    context_holder ch(&get_protocol(*clnt), &llu);
                    ch.ctx_->set_call_options( call_opt );
                    ch.ctx_->set_done_closure( done );

                    call_and_wait( call_id, llu, response, clnt, call_opt );

                } else {                  /// Send and ... just send
                    get_protocol( *clnt ).call_rpc_method( llu );
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
//                    throw vtrc::common::exception( rpc::errors::ERR_CHANNEL,
//                                                   "Clients lost");
                }

                llu->mutable_info( )->set_message_type( message_type_ );
                llu->mutable_opt( )->set_wait( false );

                return llu;
            }

            bool send_to_client( common::connection_iface_sptr  next,
                                 const common::connection_iface_sptr &sender,
                                 lowlevel_unit_type &mess,
                                 unsigned mess_type)
            {
                if( sender != next && next->active( ) ) {
                    configure_message( next, mess_type, mess );
                    get_protocol( *next ).call_rpc_method( mess );
                }
                return true;
            }


            bool send_to_client2( common::connection_iface_sptr  next,
                                  lowlevel_unit_type &mess,
                                  unsigned mess_type)
            {
                if( next->active( ) ) {
                    configure_message( next, mess_type, mess );
                    get_protocol( *next ).call_rpc_method( mess );
                }
                return true;
            }

            void send_message(lowlevel_unit_type &llu,
                    const google::protobuf::MethodDescriptor* /* method     */,
                          google::protobuf::RpcController*    /* controller */,
                    const google::protobuf::Message*          /* request    */,
                          google::protobuf::Message*          /* response   */,
                          google::protobuf::Closure* done )
            {
                common::closure_holder clhl(done);

                common::connection_iface_sptr clk(sender_.lock( ));
                vtrc::shared_ptr<common::connection_list>
                                                    lck_list(clients_.lock( ));
                if( !lck_list ) {
                    get_channel_error_callback( )( "Connection lost" );
//                    throw vtrc::common::exception( rpc::errors::ERR_CHANNEL,
//                                                   "Clients lost");
                }

                llu.mutable_info( )->set_message_type( message_type_ );
                llu.mutable_opt( )->set_wait( false );

//                const rpc_lowlevel::options &call_opt
//                          ( clk->get_protocol( ).get_method_options(method));

                if( clk ) {
                    lck_list->foreach_while(
                            vtrc::bind( &this_type::send_to_client, this,
                                         vtrc::placeholders::_1,
                                         vtrc::cref(clk), vtrc::ref(llu),
                                         message_type_) );
                } else {
                    lck_list->foreach_while(
                            vtrc::bind( &this_type::send_to_client2, this,
                                         vtrc::placeholders::_1,
                                         vtrc::ref(llu), message_type_) );
                }
            }
        };
    }

    namespace channels {

    namespace unicast {

        common::rpc_channel *create_event_channel(
                             common::connection_iface_sptr c, bool disable_wait)
        {
            static const unsigned message_type
                ( rpc::message_info::MESSAGE_SERVER_CALL );
            return new unicast_channel(c, message_type, disable_wait);
        }

        common::rpc_channel *create_callback_channel(
                             common::connection_iface_sptr c, bool disable_wait)
        {
            static const unsigned message_type
                ( rpc::message_info::MESSAGE_SERVER_CALLBACK );
            return new unicast_channel(c, message_type, disable_wait);
        }

        common::rpc_channel *create(common::connection_iface_sptr c,
                                    unsigned flags )
        {
            bool disable_wait = (flags & common::rpc_channel::DISABLE_WAIT);
            if( flags & common::rpc_channel::USE_CONTEXT_CALL ) {
                return create_callback_channel( c, disable_wait );
            } else {
                return create_event_channel( c, disable_wait );
            }
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

