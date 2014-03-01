
#include "vtrc-rpc-controller.h"

namespace vtrc { namespace common {

    namespace gpb = google::protobuf;

    struct rpc_controller::impl {

        bool failed_;
        std::string error_string_;
        bool canceled_;
        gpb::Closure *cancel_cl_;

        impl( )
            :failed_(false)
            ,canceled_(false)
            ,cancel_cl_(NULL)
        {}

        void reset( )
        {
            failed_   = false;
            canceled_ = false;
            error_string_.clear( );
        }
    };

    rpc_controller::rpc_controller( )
        :impl_(new impl)
    {}

    rpc_controller::~rpc_controller( )
    {
        delete impl_;
    }

    void rpc_controller::Reset( )
    {
        impl_->reset( );
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
