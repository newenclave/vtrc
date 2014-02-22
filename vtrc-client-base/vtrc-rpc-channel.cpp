#include "vtrc-rpc-channel.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-common/vtrc-connection-iface.h"

namespace vtrc { namespace client {

    namespace gpb = google::protobuf;

    struct rpc_channel::rpc_channel_impl {

        common::connection_iface *connection_;

        rpc_channel_impl(common::connection_iface *c)
            :connection_(c)
        {}

        void CallMethod(const gpb::MethodDescriptor* method,
                        gpb::RpcController* controller,
                        const gpb::Message* request,
                        gpb::Message* response,
                        gpb::Closure* done)
        {
            std::string service_name(method->service( )->full_name( ));
            std::string method_name(method->name( ));

            vtrc_rpc_lowlevel::lowlevel_unit llu;

            llu.mutable_call( )->set_service( service_name );
            llu.mutable_call( )->set_method( method_name );

            llu.set_request( request->SerializeAsString( ) );
            llu.set_response( response->SerializeAsString( ) );

            llu.set_id( 1000 );

            std::string serialized( llu.SerializeAsString( ) );

            connection_->send_message( serialized.c_str( ), serialized.size( ));

            if( done ) done->Run( );
        }
    };

    rpc_channel::rpc_channel( common::connection_iface *connection )
        :impl_(new rpc_channel_impl(connection))
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
