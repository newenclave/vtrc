#ifndef VTRC_COMMON_CHANNEL_H
#define VTRC_COMMON_CHANNEL_H

#include "vtrc-common/vtrc-rpc-channel.h"

namespace vtrc { namespace common {

    class common_channel: public vtrc::common::rpc_channel {

        struct impl;
        impl  *impl_;

    public:

        common_channel( );
        virtual ~common_channel( );

    private:
        void CallMethod(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller,
                        const google::protobuf::Message* request,
                        google::protobuf::Message* response,
                        google::protobuf::Closure* done);

    };

}}

#endif // VTRCCOMMONCHANNEL_H
