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

        rpc_service_wrapper( );
        rpc_service_wrapper &operator = (const rpc_service_wrapper &);
        rpc_service_wrapper (const rpc_service_wrapper &);

    public:

        typedef google::protobuf::Service       service_type;
        typedef service_type*                   service_ptr;
        typedef vtrc::shared_ptr<service_type>  service_sptr;

        typedef google::protobuf::MethodDescriptor method_type;
        typedef method_type*                       method_ptr;

        explicit rpc_service_wrapper( service_type *serv );
        explicit rpc_service_wrapper( service_sptr  serv );

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

    };

    typedef vtrc::shared_ptr<rpc_service_wrapper> rpc_service_wrapper_sptr;

}}

#endif // VTRC_RPCSERVICE_WRAPPER_H
