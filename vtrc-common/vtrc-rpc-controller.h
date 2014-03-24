#ifndef VTRC_RCP_CONTROLLER_H
#define VTRC_RCP_CONTROLLER_H

#include <google/protobuf/service.h>
#include <string>
#include "vtrc-memory.h"

namespace vtrc { namespace common {

    class rpc_controller: public google::protobuf::RpcController {

        struct impl;
        impl  *impl_;

        rpc_controller( const rpc_controller & );
        rpc_controller& operator = ( const rpc_controller & );

    public:

        rpc_controller( );
        virtual ~rpc_controller( );

    public:

        void Reset( );

        // client-side
        bool Failed( ) const;
        std::string ErrorText( ) const;
        void StartCancel( );

        // Server-side
        void SetFailed( const std::string& reason );
        bool IsCanceled( ) const;
        void NotifyOnCancel( google::protobuf::Closure* callback );
    };

    typedef vtrc::shared_ptr<rpc_controller> rpc_controller_sptr;

}}

#endif
