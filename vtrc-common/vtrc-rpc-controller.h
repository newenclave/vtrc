#ifndef VTRC_RCP_CONTROLLER_H
#define VTRC_RCP_CONTROLLER_H

#include <google/protobuf/service.h>
#include <string>

namespace vtrc { namespace common {

    class rpc_controller: public google::protobuf::RpcController {

        struct rpc_controller_impl;
        rpc_controller_impl  *impl_;

    public:

        rpc_controller( );
        ~rpc_controller( );

    private:

        // client-sede
        bool Failed( ) const;
        std::string ErrorText( ) const;
        void StartCancel( );

        // Server-side
        void SetFailed(const std::string& reason);
        bool IsCanceled() const;
        void NotifyOnCancel(google::protobuf::Closure* callback);
    };

}}

#endif
