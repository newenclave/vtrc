#ifndef VTRC_RPC_CHANNEL_H
#define VTRC_RPC_CHANNEL_H

#include <google/protobuf/service.h>

namespace vtrc_rpc_lowlevel {
    class options;
}

namespace vtrc { namespace common  {

    class rpc_channel: public google::protobuf::RpcChannel {
        struct impl;
        impl  *impl_;
    public:
        rpc_channel( );
        virtual ~rpc_channel( );
    };
}}

#endif // VTRC_RPC_CHANNEL_H
