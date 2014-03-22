#ifndef VTRC_COMMON_CHANNEL_H
#define VTRC_COMMON_CHANNEL_H

#include <stdint.h>
#include "vtrc-common/vtrc-rpc-channel.h"

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

        virtual unsigned get_call_type( ) const = 0;
        virtual uint64_t get_next_index( ) const = 0;
        virtual void send_message(  ) = 0;

    };

}}

#endif // VTRCCOMMONCHANNEL_H
