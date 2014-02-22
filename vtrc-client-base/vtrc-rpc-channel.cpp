#include "vtrc-rpc-channel.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

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
