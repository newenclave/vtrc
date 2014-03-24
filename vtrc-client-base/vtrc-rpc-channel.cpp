#include "vtrc-rpc-channel.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-protocol-layer.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-closure-holder.h"

namespace vtrc { namespace client {

    namespace gpb = google::protobuf;

    namespace {
        typedef vtrc::shared_ptr <
            vtrc_rpc_lowlevel::lowlevel_unit
        > lowlevel_unit_sptr;
    }

    struct rpc_channel::impl {

        vtrc::weak_ptr<common::connection_iface> connection_;

        rpc_channel *parent_;

        impl(vtrc::shared_ptr<common::connection_iface> c)
            :connection_(c)
        {}

        static bool waitable_call( const lowlevel_unit_sptr &llu )
        {
            const bool  has = llu->info( ).has_wait_for_response( );
            return !has || (has && llu->info( ).wait_for_response( ));
        }

        void CallMethod(const gpb::MethodDescriptor* method,
                        gpb::RpcController* controller,
                        const gpb::Message* request,
                        gpb::Message* response,
                        gpb::Closure* done)
        {
            //common::closure_holder hold(done);

            common::connection_iface_sptr cl(connection_.lock( ));

            if( cl.get( ) == NULL ) {
                throw vtrc::common::exception( vtrc_errors::ERR_CHANNEL,
                                               "Connection lost");
            }

            vtrc_rpc_lowlevel::options call_opt
                             ( cl->get_protocol( ).get_method_options(method) );

            std::string service_name(method->service( )->full_name( ));
            std::string method_name(method->name( ));

            lowlevel_unit_sptr llu(
                        vtrc::make_shared<vtrc_rpc_lowlevel::lowlevel_unit>( ));

            llu->mutable_call( )->set_service( service_name );
            llu->mutable_call( )->set_method( method_name );

            llu->set_request( request->SerializeAsString( ) );
            llu->set_response( response->SerializeAsString( ) );

            llu->mutable_info( )->set_message_type(
                               vtrc_rpc_lowlevel::message_info::MESSAGE_CALL );

            uint64_t call_id = cl->get_protocol( ).next_index( );

            llu->set_id( call_id );

            if( call_opt.wait( ) && waitable_call( llu ) ) { // WAITABLE CALL

                cl->get_protocol( ).call_rpc_method( call_id, *llu );

                std::deque<lowlevel_unit_sptr> data_list;

                cl->get_protocol( ).wait_call_slot( call_id, data_list,
                                                    call_opt.call_timeout( ) );

                lowlevel_unit_sptr top( data_list.front( ) );

                if( top->error( ).code( ) != vtrc_errors::ERR_NO_ERROR ) {
                    cl->get_protocol( ).close_slot( call_id );
                    throw vtrc::common::exception( top->error( ).code( ),
                                                 top->error( ).category( ),
                                                 top->error( ).additional( ) );
                }

                response->ParseFromString( top->response( ) );

                cl->get_protocol( ).close_slot( call_id );

            } else { // NOT WAITABLE CALL
                cl->get_protocol( ).call_rpc_method( *llu );
            }
        }
    };

    rpc_channel::rpc_channel( common::connection_iface_sptr c )
        :impl_(new impl(c))
    {
        impl_->parent_ = this;
    }

    rpc_channel::~rpc_channel( )
    {
        delete impl_;
    }

    void rpc_channel::CallMethod(const gpb::MethodDescriptor* method,
                    gpb::RpcController* controller,
                    const gpb::Message* request,
                    gpb::Message* response,
                    gpb::Closure* done)
    {
        impl_->CallMethod( method, controller, request, response, done );
    }

}}
