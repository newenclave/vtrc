#ifndef VTRC_RCP_CHANNEL_H
#define VTRC_RCP_CHANNEL_H

#include "vtrc-common/vtrc-rpc-channel.h"
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

    class rpc_channel_c: public vtrc::common::rpc_channel {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        rpc_channel_c & operator = ( const rpc_channel_c & );
        rpc_channel_c( );

    public:

        explicit rpc_channel_c(vtrc::shared_ptr<common::connection_iface> conn);

        rpc_channel_c(vtrc::shared_ptr<common::connection_iface> conn,
                      bool disable_wait);

        rpc_channel_c(vtrc::shared_ptr<common::connection_iface> conn,
                      bool disable_wait,
                      bool make_insertion_call);

        ~rpc_channel_c( );

    private:

        void CallMethod(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller,
                        const google::protobuf::Message* request,
                        google::protobuf::Message* response,
                        google::protobuf::Closure* done);
    };

    typedef vtrc::shared_ptr<rpc_channel_c> rpc_channel_c_sptr;

}}

#endif // VTRCRCPCHANNEL_H
