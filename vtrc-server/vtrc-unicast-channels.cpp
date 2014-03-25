#include "vtrc-unicast-channels.h"
#include "vtrc-protocol-layer-s.h"

#include "vtrc-common/vtrc-call-context.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"


namespace vtrc { namespace server {

    namespace {

        namespace gpb = google::protobuf;

        typedef vtrc_rpc_lowlevel::lowlevel_unit     lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        class unicast_channel: public common_channel {

            common::connection_iface_wptr client_;
            const unsigned                message_type_;

        public:

            unicast_channel( common::connection_iface_sptr c,
                             unsigned mess_type)
                :client_(c)
                ,message_type_(mess_type)
            {}

            static
            gpb::uint64 get_callback_index( common::connection_iface_sptr cl )
            {
                const common::call_context
                                  *c(cl->get_protocol( ).get_call_context( ));

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

                vtrc_rpc_lowlevel::options call_opt
                            ( clk->get_protocol( ).get_method_options(method) );

                llu->mutable_info( )->set_message_type(message_type_);

                gpb::uint64 call_id = get_mess_index(clk, message_type_);

                llu->set_id(call_id);

                //// WAITABLE CALL
                if( call_opt.wait( ) && llu->info( ).wait_for_response( ) ) {

                    std::cout << "test event send "
                              << call_id << "\n";

                    clk->get_protocol( ).call_rpc_method( call_id, *llu );

                    std::deque<lowlevel_unit_sptr> data_list;

                    clk->get_protocol( ).read_slot_for( call_id, data_list,
                                                   call_opt.call_timeout( ) );

                    lowlevel_unit_sptr top( data_list.front( ) );

                    if( top->error( ).code( ) != vtrc_errors::ERR_NO_ERROR ) {

                        clk->get_protocol( ).erase_slot( call_id );
                        throw vtrc::common::exception( top->error( ).code( ),
                                                 top->error( ).category( ),
                                                 top->error( ).additional( ) );
                    }

                    response->ParseFromString( top->response( ) );

                    clk->get_protocol( ).erase_slot( call_id );

                } else { // NOT WAITABLE CALL
                    clk->get_protocol( ).call_rpc_method( *llu );
                }

            }
        };

    }

    namespace channels { namespace unicast {

        common_channel *create_event_channel( common::connection_iface_sptr c )
        {
            static const unsigned message_type
                (vtrc_rpc_lowlevel::message_info::MESSAGE_EVENT);
            return new unicast_channel(c, message_type);
        }

        common_channel *create_callback_channel(common::connection_iface_sptr c)
        {
            static const unsigned message_type
                (vtrc_rpc_lowlevel::message_info::MESSAGE_CALLBACK);
            return new unicast_channel(c, message_type);
        }

    }}

}}

