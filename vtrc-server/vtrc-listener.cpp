#include "vtrc-listener.h"
#include "vtrc-application.h"

#include "vtrc-common/vtrc-enviroment.h"

namespace vtrc { namespace server {

    struct listener::impl {
        application         &app_;
        common::enviroment   env_;
        listener_options     opts_;

        listener       *parent_;

        impl( application &app, const listener_options &opts )
            :app_(app)
            ,env_(app_.get_enviroment( ))
            ,opts_(opts)
        { }
    };

    listener::listener(application & app,
                                 const listener_options &opts)
        :impl_(new impl(app, opts))
    {
        impl_->parent_ = this;
    }

    listener::~listener( )
    {
        delete impl_;
    }

    application &listener::get_application( )
    {
        return impl_->app_;
    }

    common::enviroment  &listener::get_enviroment( )
    {
        return impl_->env_;
    }

    const listener_options &listener::get_options( ) const
    {
        return impl_->opts_;
    }


    namespace listeners {

        listener_options default_options( )
        {
            listener_options def_opts = { 5, 1024 * 1024, 20, 4096 };
            return def_opts;
        }

    }

}}
