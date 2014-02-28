#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "vtrc-rpc-service-wrapper.h"

namespace vtrc { namespace common {

    namespace gpb = google::protobuf;

    struct rpc_service_wrapper::impl {
        gpb::Service *service_;

        impl( gpb::Service *s )
            :service_(s)
        {}

        ~impl( )
        {
            delete service_;
        }

        const std::string &name( )
        {
            return service_->GetDescriptor()->full_name( );
        }

        const gpb::MethodDescriptor *find_method( const std::string &name) const
        {
            return service_->GetDescriptor( )->FindMethodByName( name );
        }

        gpb::Service *service( ) { return service_; }
        const gpb::Service *service( ) const { return service_; }

    };

    rpc_service_wrapper::rpc_service_wrapper( gpb::Service *service )
        :impl_(new impl(service))
    {}

    rpc_service_wrapper::~rpc_service_wrapper( )
    {
        delete impl_;
    }

    const std::string &rpc_service_wrapper::name( )
    {
        return impl_->name( );
    }

    const gpb::MethodDescriptor *rpc_service_wrapper::get_method (
                                                  const std::string &name) const
    {
        return find_method( name );
    }

    gpb::Service *rpc_service_wrapper::service( )
    {
        return impl_->service( );
    }

    const gpb::Service *rpc_service_wrapper::service( ) const
    {
        return impl_->service( );
    }

    const gpb::MethodDescriptor *rpc_service_wrapper::find_method(
                                                  const std::string &name) const
    {
        return impl_->find_method( name );
    }

}}
