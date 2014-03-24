#ifndef VTRC_RPC_CHANNEL_H
#define VTRC_RPC_CHANNEL_H

#include <google/protobuf/service.h>
#include "vtrc-memory.h"

namespace vtrc_rpc_lowlevel {
    class lowlevel_unit;
    class options;
}

namespace vtrc { namespace common  {

    class rpc_channel: public google::protobuf::RpcChannel {

        struct impl;
        impl  *impl_;

        rpc_channel( const rpc_channel & );
        rpc_channel&  operator = ( const rpc_channel & );

    public:

        typedef vtrc_rpc_lowlevel::lowlevel_unit     lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        rpc_channel( );
        virtual ~rpc_channel( );
    public:

        lowlevel_unit_sptr create_lowlevel(
                          const google::protobuf::MethodDescriptor* method,
                          const google::protobuf::Message* request,
                                google::protobuf::Message* response) const;

    };
}}

#endif // VTRC_RPC_CHANNEL_H
