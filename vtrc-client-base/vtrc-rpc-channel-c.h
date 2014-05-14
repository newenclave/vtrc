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

namespace vtrc_rpc {
    class lowlevel_unit;
}

namespace vtrc {

    namespace common {
        struct connection_iface;
    }

namespace client {

    class rpc_channel_c: public common::rpc_channel {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        rpc_channel_c & operator = ( const rpc_channel_c & );
        rpc_channel_c( );

    public:

        explicit rpc_channel_c(vtrc::shared_ptr<common::connection_iface> conn);

        rpc_channel_c(vtrc::shared_ptr<common::connection_iface> conn,
                      common::rpc_channel::options opts );

        ~rpc_channel_c( );

        bool alive( ) const;

    private:

        void configure_message_for( common::connection_iface_sptr c,
                                lowlevel_unit_type &llu ) const;

        void send_message( lowlevel_unit_type &llu,
                    const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                          google::protobuf::Message* response,
                          google::protobuf::Closure* done );

    };

    typedef vtrc::shared_ptr<rpc_channel_c> rpc_channel_c_sptr;

}}

#endif // VTRCRCPCHANNEL_H
