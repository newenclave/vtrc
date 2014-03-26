#include "vtrc-unicast-channels.h"
#include "vtrc-protocol-layer-s.h"

#include "vtrc-common/vtrc-call-context.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-chrono.h"

namespace vtrc { namespace server {

    namespace {

        namespace gpb = google::protobuf;

        typedef vtrc_rpc_lowlevel::lowlevel_unit     lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

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

            static
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

            static
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

    }

    namespace channels { namespace unicast {

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

    }}

}}

