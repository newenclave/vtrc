#ifndef VTRC_RPCSERVICE_WRAPPER_H
#define VTRC_RPCSERVICE_WRAPPER_H

#include <string>
#include "vtrc-memory.h"

namespace google { namespace protobuf {
    class Service;
    class MethodDescriptor;
    class Message;
    class RpcController;
    class Closure;
}}

namespace vtrc { namespace common {

    class rpc_service_wrapper {

        struct impl;
        impl  *impl_;

        VTRC_DISABLE_COPY_DEFAULT( rpc_service_wrapper );

    public:

        typedef vtrc::shared_ptr<rpc_service_wrapper>   shared_type;
        typedef google::protobuf::Service               service_type;
        typedef service_type*                           service_ptr;
        typedef vtrc::shared_ptr<service_type>          service_sptr;

        typedef google::protobuf::MethodDescriptor method_type;
        typedef method_type*                       method_ptr;

        explicit rpc_service_wrapper( service_type *serv );
        explicit rpc_service_wrapper( service_sptr  serv );

        static
        shared_type wrap( service_sptr  serv )
        {
            return vtrc::make_shared<rpc_service_wrapper>( serv );
        }

        static
        shared_type wrap( service_type *serv )
        {
            return vtrc::make_shared<rpc_service_wrapper>( serv );
        }

        virtual ~rpc_service_wrapper( );

        /// names for protocol
        virtual const std::string &name( );
        virtual const method_type *get_method( const std::string &name ) const;
        virtual void call_method( const method_type *method,
                             google::protobuf::RpcController* controller,
                             const google::protobuf::Message* request,
                             google::protobuf::Message* response,
                             google::protobuf::Closure* done );

        google::protobuf::Service       *service( );
        const google::protobuf::Service *service( ) const;


    protected:

        const method_type *find_method ( const std::string &name ) const;
        void call_default( const method_type *method,
                           google::protobuf::RpcController* controller,
                           const google::protobuf::Message* request,
                           google::protobuf::Message* response,
                           google::protobuf::Closure* done );

    };

    typedef vtrc::shared_ptr<rpc_service_wrapper> rpc_service_wrapper_sptr;

}}

#endif // VTRC_RPCSERVICE_WRAPPER_H
