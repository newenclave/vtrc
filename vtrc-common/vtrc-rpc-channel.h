#ifndef VTRC_RPC_CHANNEL_H
#define VTRC_RPC_CHANNEL_H

#include <google/protobuf/service.h>
#include "vtrc-connection-iface.h"
#include "vtrc-protocol-layer.h"

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

        void process_waitable_call( google::protobuf::uint64 call_id,
                            lowlevel_unit_sptr &llu,
                            google::protobuf::Message* response,
                            common::connection_iface_sptr &cl,
                            const vtrc_rpc_lowlevel::options &call_opt ) const;
    protected:
        typedef protocol_layer::context_holder context_holder;

    };
}}

#endif // VTRC_RPC_CHANNEL_H
