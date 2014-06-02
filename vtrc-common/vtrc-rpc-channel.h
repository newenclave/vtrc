#ifndef VTRC_RPC_CHANNEL_H
#define VTRC_RPC_CHANNEL_H

#include "google/protobuf/service.h"

#include "vtrc-connection-iface.h"
#include "vtrc-protocol-layer.h"

namespace vtrc_rpc {
    class lowlevel_unit;
    class options;
}

namespace vtrc { namespace common  {

    class rpc_channel: public google::protobuf::RpcChannel {

        rpc_channel( const rpc_channel & );
        rpc_channel&  operator = ( const rpc_channel & );

        unsigned direct_call_type_;
        unsigned callback_type_;

    public:

        enum flags {
             DEFAULT            = 0
            ,DISABLE_WAIT       = 1
            ,USE_CONTEXT_CALL   = 1 << 1
            //,CONTEXT_REQIRED    = 1 << 2
        };

        typedef vtrc_rpc::lowlevel_unit              lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        rpc_channel(unsigned direct_call_type, unsigned callback_type);
        virtual ~rpc_channel( );

    public:

        virtual bool alive( ) const = 0;

        lowlevel_unit_sptr create_lowlevel(
                          const google::protobuf::MethodDescriptor* method,
                          const google::protobuf::Message* request,
                                google::protobuf::Message* response) const;

        void call_and_wait( google::protobuf::uint64 call_id,
                            const lowlevel_unit_type &llu,
                            google::protobuf::Message* response,
                            common::connection_iface_sptr &cl,
                            const vtrc_rpc::options &call_opt ) const;
    protected:

        typedef protocol_layer::context_holder context_holder;

        protocol_layer &get_protocol( common::connection_iface &cl );

        void configure_message( common::connection_iface_sptr c,
                                unsigned specified_call_type,
                                lowlevel_unit_type &llu ) const;

        void CallMethod(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller,
                        const google::protobuf::Message* request,
                        google::protobuf::Message* response,
                        google::protobuf::Closure* done);

        ///
        /// 'send' implementation for children
        ///
        virtual void send_message( lowlevel_unit_type &llu,
                    const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                          google::protobuf::Message* response,
                          google::protobuf::Closure* done ) = 0;

    };
}}

#endif // VTRC_RPC_CHANNEL_H
