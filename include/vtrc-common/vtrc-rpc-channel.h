#ifndef VTRC_RPC_CHANNEL_H
#define VTRC_RPC_CHANNEL_H

#include "google/protobuf/service.h"

#include "vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure.h"

#include "vtrc-stdint.h"

namespace vtrc { namespace rpc {
    class lowlevel_unit;
    class options;
}}

namespace vtrc { namespace common  {

    class rpc_channel: public google::protobuf::RpcChannel {

        rpc_channel( const rpc_channel & );
        rpc_channel&  operator = ( const rpc_channel & );

        struct  impl;
        impl   *impl_;

    public:

        enum flags {
             DEFAULT            = 0
            ,DISABLE_WAIT       = 1
            ,USE_CONTEXT_CALL   = 1 << 1
            ,STATIC_CONTEXT     = 1 << 2
        };

        typedef rpc::lowlevel_unit                   lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        typedef vtrc::function<
            void (unsigned code, unsigned cat, const char *mess)
        > proto_error_cb_type;

        typedef vtrc::function<void (const char *)> channel_error_cb_type;

        rpc_channel( unsigned direct_call_type, unsigned callback_type );
        virtual ~rpc_channel( );

        virtual void set_flags( unsigned /*flags*/ );
        virtual unsigned get_flags( ) const;

        virtual bool alive( ) const = 0;

        static
        lowlevel_unit_sptr create_lowlevel(
                          const google::protobuf::MethodDescriptor* method,
                          const google::protobuf::Message* request,
                                google::protobuf::Message* response );

        virtual
        lowlevel_unit_sptr make_lowlevel(
                            const google::protobuf::MethodDescriptor* method,
                            const google::protobuf::Message* request,
                                  google::protobuf::Message* response );


        virtual lowlevel_unit_sptr raw_call( lowlevel_unit_sptr llu,
                                   common::lowlevel_closure_type callbacks );

        void set_channel_data( const std::string &data );
        const std::string &channel_data( );

        const proto_error_cb_type &get_proto_error_callback( ) const;
        void set_proto_error_callback( const proto_error_cb_type &value );

        const channel_error_cb_type &get_channel_error_callback( ) const;
        void set_channel_error_callback( const channel_error_cb_type &value );

    protected:

        /// true == success
        bool call_and_wait( google::protobuf::uint64 call_id,
                            const lowlevel_unit_type &llu,
                            google::protobuf::Message* response,
                            common::connection_iface_sptr &cl,
                            const rpc::options *call_opt ) const;

        lowlevel_unit_sptr call_and_wait_raw(google::protobuf::uint64 call_id,
                            lowlevel_unit_type &llu,
                            common::connection_iface_sptr &cl,
                            common::lowlevel_closure_type events,
                            const rpc::options *call_opt ) const;

        //typedef protocol_layer::context_holder context_holder;

        protocol_layer &get_protocol( common::connection_iface &cl );

        void configure_message( common::connection_iface_sptr c,
                                unsigned specified_call_type,
                                lowlevel_unit_type &llu ) const;

        void configure_message( vtrc::uint64_t target_id,
                                unsigned mess_type,
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
