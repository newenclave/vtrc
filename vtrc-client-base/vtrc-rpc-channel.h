#ifndef VTRC_RCP_CHANNEL_H
#define VTRC_RCP_CHANNEL_H

#include <google/protobuf/service.h>
#include "vtrc-memory.h"

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

        struct impl;
        impl  *impl_;

        rpc_channel & operator = ( const rpc_channel & );
        rpc_channel( );

    public:

        explicit rpc_channel( shared_ptr<common::connection_iface> conn);
        ~rpc_channel( );

    private:

        void CallMethod(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller,
                        const google::protobuf::Message* request,
                        google::protobuf::Message* response,
                        google::protobuf::Closure* done);
    };

    typedef shared_ptr<rpc_channel> rpc_channel_sptr;

}}

#endif // VTRCRCPCHANNEL_H
