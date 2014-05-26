#include "vtrc-listener.h"
#include "vtrc-application.h"

#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-atomic.h"

namespace vtrc { namespace server {

    struct listener::impl {
        application         &app_;
        common::enviroment   env_;
        listener_options     opts_;

        listener            *parent_;
        vtrc::atomic<size_t> client_count_;

        impl( application &app, const listener_options &opts )
            :app_(app)
            ,env_(app_.get_enviroment( ))
            ,opts_(opts)
            ,client_count_(0)
        { }
    };

    listener::listener(application & app, const listener_options &opts)
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

    size_t listener::clients_count( ) const
    {
        return impl_->client_count_;
    }

    vtrc::weak_ptr<listener> listener::weak_from_this( )
    {
        return vtrc::weak_ptr<listener>( shared_from_this( ) );
    }

    vtrc::weak_ptr<const listener> listener::weak_from_this( ) const
    {
        return vtrc::weak_ptr<listener const>( shared_from_this( ));
    }

    void listener::new_connection( const common::connection_iface *conn )
    {
        ++impl_->client_count_;
        on_new_connection_( conn );
    }

    void listener::stop_connection( const common::connection_iface *conn )
    {
        --impl_->client_count_;
        on_stop_connection_( conn );
    }

    namespace listeners {

        listener_options default_options( )
        {
            listener_options def_opts = { 0 };

            def_opts.maximum_active_calls   = 5;
            def_opts.maximum_message_length = 1024 * 1024;
            def_opts.maximum_total_calls    = 20;
            def_opts.read_buffer_size       = 4096;
            def_opts.maximum_stack_size     = 64;

            return def_opts;
        }

    }

}}
