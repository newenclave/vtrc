#include "vtrc-channels.h"
#include "vtrc-protocol-layer-s.h"

#include "vtrc-common/vtrc-call-context.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

//#include "vtrc-chrono.h"
#include "vtrc-bind.h"

namespace vtrc { namespace server {

    namespace {

        namespace gpb = google::protobuf;

        typedef vtrc_rpc_lowlevel::lowlevel_unit     lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        gpb::uint64 get_callback_index( common::connection_iface_sptr cl )
        {
            const common::call_context
                                  *c(common::call_context::get(cl.get( )));

            if( c ) {
                return c->get_lowlevel_message( )->id( );
            } else {
                return cl->get_protocol( ).next_index( );
            }
        }

        gpb::uint64 get_mess_index( common::connection_iface_sptr c,
                                    unsigned mess_type)
        {
            switch( mess_type ) {
            case vtrc_rpc_lowlevel::message_info::MESSAGE_CALLBACK:
                return get_callback_index( c );
            case vtrc_rpc_lowlevel::message_info::MESSAGE_EVENT:
                return c->get_protocol( ).next_index( );
            };
        }

        class unicast_channel: public common_channel {

            common::connection_iface_wptr client_;
            const unsigned                message_type_;
            const bool                    disable_wait_;

        public:

            unicast_channel( common::connection_iface_sptr c,
                             unsigned mess_type, bool disable_wait)
                :client_(c)
                ,message_type_(mess_type)
                ,disable_wait_(disable_wait)
            {}

            void send_message(lowlevel_unit_sptr llu,
                        const google::protobuf::MethodDescriptor* method,
                              google::protobuf::RpcController* controller,
                        const google::protobuf::Message* /*request*/,
                              google::protobuf::Message* response,
                              google::protobuf::Closure* done )
            {
                common::closure_holder clhl(done);
                common::connection_iface_sptr clk(client_.lock( ));

                if( clk.get( ) == NULL ) {
                    throw vtrc::common::exception( vtrc_errors::ERR_CHANNEL,
                                                   "Connection lost");
                }

                const vtrc_rpc_lowlevel::options &call_opt
                            ( clk->get_protocol( ).get_method_options(method) );

                llu->mutable_info( )->set_message_type(message_type_);

                gpb::uint64 call_id = get_mess_index(clk, message_type_);

                llu->set_id(call_id);

                if( disable_wait_ )
                    llu->mutable_opt( )->set_wait(false);

                //// WAITABLE CALL
                if( call_opt.wait( ) && llu->opt( ).wait( ) ) {

                    process_waitable_call( call_id, llu, response,
                                           clk, call_opt );

                } else { // NOT WAITABLE CALL
                    clk->get_protocol( ).call_rpc_method( *llu );
//                    clk->get_protocol( ).call_rpc_method( llu->id( ), *llu );
//                    clk->get_protocol( ).erase_slot( llu->id( ) );
                }

            }
        };

        class broadcast_channel: public common_channel {

            typedef broadcast_channel               this_type;
            vtrc::weak_ptr<common::connection_list> clients_;
            common::connection_iface_wptr           sender_;
            const unsigned                          message_type_;

        public:

            broadcast_channel( vtrc::weak_ptr<common::connection_list> clients,
                               unsigned mess_type)
                :clients_(clients)
                ,message_type_(mess_type)
            { }

            broadcast_channel( vtrc::weak_ptr<common::connection_list> clients,
                               unsigned mess_type,
                               common::connection_iface_wptr   sender)
                :clients_(clients)
                ,message_type_(mess_type)
                ,sender_(sender)
            { }

            static
            bool send_to_client( common::connection_iface_sptr next,
                                 common::connection_iface_sptr sender,
                                 lowlevel_unit_sptr mess)
            {
                if( !sender || (sender != next) ) {
                    mess->set_id( get_mess_index( next,
                                               mess->info( ).message_type( )));
                    next->get_protocol( ).call_rpc_method( *mess );
                }
                return true;
            }

            void send_message(lowlevel_unit_sptr llu,
                        const google::protobuf::MethodDescriptor* method,
                              google::protobuf::RpcController* controller,
                        const google::protobuf::Message* /*request*/,
                              google::protobuf::Message* response,
                              google::protobuf::Closure* done )
            {
                common::closure_holder clhl(done);
                common::connection_iface_sptr clk(sender_.lock( ));
                vtrc::shared_ptr<common::connection_list>
                                                    lck_list(clients_.lock( ));
                if( !lck_list ) {
                    throw vtrc::common::exception( vtrc_errors::ERR_CHANNEL,
                                                   "Clients lost");
                }

                llu->mutable_info( )->set_message_type( message_type_ );
                llu->mutable_opt( )->set_wait( false );

//                const vtrc_rpc_lowlevel::options &call_opt
//                            ( clk->get_protocol( ).get_method_options(method));

                lck_list->foreach_while(
                            vtrc::bind( &this_type::send_to_client, _1,
                                        clk, llu) );

            }
        };
    }

    namespace channels {

    namespace unicast {

        common_channel *create_event_channel( common::connection_iface_sptr c,
                                              bool disable_wait)
        {
            static const unsigned message_type
                (vtrc_rpc_lowlevel::message_info::MESSAGE_EVENT);
            return new unicast_channel(c, message_type, disable_wait);
        }

        common_channel *create_callback_channel(common::connection_iface_sptr c,
                                                bool disable_wait)
        {
            static const unsigned message_type
                (vtrc_rpc_lowlevel::message_info::MESSAGE_CALLBACK);
            return new unicast_channel(c, message_type, disable_wait);
        }


    }

    namespace broadcast {

        common_channel *create_event_channel(
                                vtrc::shared_ptr<common::connection_list> cl,
                                common::connection_iface_sptr c )
        {
            static const unsigned message_type
                (vtrc_rpc_lowlevel::message_info::MESSAGE_EVENT);

            return new broadcast_channel(cl->weak_from_this( ),
                                         message_type, c->weak_from_this( ));

        }

        common_channel *create_event_channel(
                                vtrc::shared_ptr<common::connection_list> cl )
        {
            static const unsigned message_type
                (vtrc_rpc_lowlevel::message_info::MESSAGE_EVENT);
            return new broadcast_channel(cl->weak_from_this( ), message_type );
        }

        common_channel *create_callback_channel(
                               vtrc::shared_ptr<common::connection_list> cl,
                               common::connection_iface_sptr c )
        {
            static const unsigned message_type
                (vtrc_rpc_lowlevel::message_info::MESSAGE_CALLBACK);
            return new broadcast_channel(cl->weak_from_this( ),
                                         message_type, c->weak_from_this( ));
        }

        common_channel *create_callback_channel(
                                 vtrc::shared_ptr<common::connection_list> cl)
        {
            static const unsigned message_type
                (vtrc_rpc_lowlevel::message_info::MESSAGE_CALLBACK);
            return new broadcast_channel(cl->weak_from_this( ), message_type );

        }
    }

    }

}}
