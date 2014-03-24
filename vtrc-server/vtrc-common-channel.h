#ifndef VTRC_COMMON_CHANNEL_H
#define VTRC_COMMON_CHANNEL_H

#include <stdint.h>
#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-memory.h"

namespace vtrc_rpc_lowlevel {
    class lowlevel_unit;
    class options;
}

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
        //

        virtual void send_message(
                 vtrc::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit> llu ) = 0;

    };

}}

#endif // VTRCCOMMONCHANNEL_H
