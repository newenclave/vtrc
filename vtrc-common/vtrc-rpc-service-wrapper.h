#ifndef VTRC_RPCSERVICE_WRAPPER_H
#define VTRC_RPCSERVICE_WRAPPER_H

#include <string>
#include "vtrc-memory.h"

namespace google { namespace protobuf {
    class Service;
    class MethodDescriptor;
}}

namespace vtrc { namespace common {

    class rpc_service_wrapper {

        struct impl;
        impl  *impl_;

        rpc_service_wrapper( );
        rpc_service_wrapper &operator = (const rpc_service_wrapper &);

    public:

        explicit rpc_service_wrapper( google::protobuf::Service *serv );
        explicit rpc_service_wrapper(
                     vtrc::shared_ptr<google::protobuf::Service> serv );

        virtual ~rpc_service_wrapper( );

        /// names for protocol
        virtual const std::string &name( );
        virtual const google::protobuf::MethodDescriptor *get_method (
                                                const std::string &name ) const;

        google::protobuf::Service *service( );
        const google::protobuf::Service *service( ) const;

    protected:

        const google::protobuf::MethodDescriptor *find_method (
                                                const std::string &name ) const;

    };

    typedef vtrc::shared_ptr<rpc_service_wrapper> rpc_service_wrapper_sptr;

}}

#endif // VTRC_RPCSERVICE_WRAPPER_H
