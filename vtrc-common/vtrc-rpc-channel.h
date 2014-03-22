#ifndef VTRC_RPC_CHANNEL_H
#define VTRC_RPC_CHANNEL_H

#include <google/protobuf/service.h>

namespace vtrc { namespace common  {

    class rpc_channel: public google::protobuf::RpcChannel {
        struct impl;
        impl  *impl_;

        rpc_channel( const rpc_channel & );
        rpc_channel&  operator = ( const rpc_channel & );

    public:
        rpc_channel( );
        virtual ~rpc_channel( );
    };
}}

#endif // VTRC_RPC_CHANNEL_H
