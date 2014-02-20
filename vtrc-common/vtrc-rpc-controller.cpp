
#include "vtrc-rpc-controller.h"

namespace vtrc { namespace common {

    namespace gpb = google::protobuf;

    struct rpc_controller::rpc_controller_impl {

        bool failed_;
        std::string error_string_;
        bool canceled_;
        gpb::Closure *cancel_cl_;

        rpc_controller_impl( )
            :failed_(false)
            ,canceled_(false)
            ,cancel_cl_(NULL)
        {}
    };

    rpc_controller::rpc_controller( )
        :impl_(new rpc_controller_impl)
    {}

    rpc_controller::~rpc_controller( )
    {
        delete impl_;
    }

    // client-sede
    bool rpc_controller::Failed( ) const
    {
        return impl_->failed_;
    }

    std::string rpc_controller::ErrorText( ) const
    {
        return impl_->error_string_;
    }

    void rpc_controller::StartCancel( )
    {
        impl_->canceled_ = true;
        if( impl_->cancel_cl_ )
            impl_->cancel_cl_->Run( );
    }

    // Server-side
    void rpc_controller::SetFailed( const std::string& reason )
    {
        impl_->failed_ = true;
        impl_->error_string_.assign( reason );
    }

    bool rpc_controller::IsCanceled( ) const
    {
        return impl_->canceled_;
    }

    void rpc_controller::NotifyOnCancel( gpb::Closure* callback )
    {
        impl_->cancel_cl_ = callback;
    }

}}
