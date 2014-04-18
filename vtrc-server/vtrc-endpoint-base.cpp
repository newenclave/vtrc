#include "vtrc-endpoint-base.h"
#include "vtrc-application.h"

#include "vtrc-common/vtrc-enviroment.h"

namespace vtrc { namespace server {

    struct endpoint_base::impl {
        application         &app_;
        common::enviroment   env_;
        endpoint_options     opts_;

        endpoint_base       *parent_;

        impl( application &app, const endpoint_options &opts )
            :app_(app)
            ,env_(app_.get_enviroment( ))
            ,opts_(opts)
        { }
    };

    endpoint_base::endpoint_base(application & app,
                                 const endpoint_options &opts)
        :impl_(new impl(app, opts))
    {
        impl_->parent_ = this;
    }

    endpoint_base::~endpoint_base( )
    {
        delete impl_;
    }

    application &endpoint_base::get_application( )
    {
        return impl_->app_;
    }

    common::enviroment  &endpoint_base::get_enviroment( )
    {
        return impl_->env_;
    }

    const endpoint_options &endpoint_base::get_options( ) const
    {
        return impl_->opts_;
    }

}}
