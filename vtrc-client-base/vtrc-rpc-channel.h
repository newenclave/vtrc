#ifndef VTRC_RCP_CHANNEL_H
#define VTRC_RCP_CHANNEL_H

#include <google/protobuf/service.h>
#include <boost/shared_ptr.hpp>

namespace google { namespace protobuf {
    class Message;
    class RpcController;
    class Closure;
    class MethodDescriptor;
}}

namespace vtrc {

namespace common {
    struct connection_iface;
}

namespace client {

    class rpc_channel: public google::protobuf::RpcChannel {

        struct rpc_channel_impl;
        rpc_channel_impl  *impl_;

    public:

        rpc_channel( common::connection_iface *connection );
        ~rpc_channel( );

    private:

        void CallMethod(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller,
                        const google::protobuf::Message* request,
                        google::protobuf::Message* response,
                        google::protobuf::Closure* done);
    };

    typedef boost::shared_ptr<rpc_channel> rpc_channel_sptr;

}}

#endif // VTRCRCPCHANNEL_H
