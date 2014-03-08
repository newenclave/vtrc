#include "vtrc-rpc-channel.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-protocol-layer.h"

namespace vtrc { namespace client {

    namespace gpb = google::protobuf;

    struct rpc_channel::impl {

        weak_ptr<common::connection_iface> connection_;

        impl(shared_ptr<common::connection_iface> c)
            :connection_(c)
        {}

        void CallMethod(const gpb::MethodDescriptor* method,
                        gpb::RpcController* controller,
                        const gpb::Message* request,
                        gpb::Message* response,
                        gpb::Closure* done)
        {

            common::connection_iface_sptr cl(connection_.lock());

            if( cl.get( ) == NULL )
                throw std::runtime_error( "Channel is empty" );

            std::string service_name(method->service( )->full_name( ));
            std::string method_name(method->name( ));

            shared_ptr<
                    vtrc_rpc_lowlevel::lowlevel_unit
            > llu(new vtrc_rpc_lowlevel::lowlevel_unit);

            llu->mutable_call( )->set_service( service_name );
            llu->mutable_call( )->set_method( method_name );

            llu->set_request( request->SerializeAsString( ) );
            llu->set_response( response->SerializeAsString( ) );

            llu->mutable_info( )->set_message_type(
                        vtrc_rpc_lowlevel::message_info::MESSAGE_CALL );

            uint64_t call_id = cl->get_protocol( ).next_index( );

            llu->set_id( call_id );

            cl->get_protocol( ).call_rpc_method( call_id, *llu );

            std::deque< shared_ptr <
                        vtrc_rpc_lowlevel::lowlevel_unit
                      >
            > data_list;

            cl->get_protocol( ).wait_call_slot( call_id, data_list, 2000 );

            response->ParseFromString( data_list.front( )->response( ) );

            cl->get_protocol( ).close_slot( call_id );

            if( done ) done->Run( );
        }
    };

    rpc_channel::rpc_channel( common::connection_iface_sptr c )
        :impl_(new impl(c))
    {}

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
