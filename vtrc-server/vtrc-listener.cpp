#include "vtrc-listener.h"
#include "vtrc-application.h"

#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-atomic.h"

#include "protocol/vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace server {

    struct listener::impl {
        application              &app_;
        common::enviroment        env_;
        vtrc_rpc::session_options opts_;

        listener            *parent_;
        vtrc::atomic<size_t> client_count_;

        impl( application &app, const vtrc_rpc::session_options &opts )
            :app_(app)
            ,env_(app_.get_enviroment( ))
            ,opts_(opts)
            ,client_count_(0)
        { }
    };

    listener::listener(application & app, const vtrc_rpc::session_options &opts)
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

    const vtrc_rpc::session_options &listener::get_options( ) const
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

    void listener::call_on_accept_failed( const boost::system::error_code &err )
    {
        on_accept_failed_( err );
    }

    void listener::call_on_stop()
    {
        on_stop_( );
    }

    void listener::call_on_start( )
    {
        on_start_( );
    }

    namespace listeners {
        vtrc_rpc::session_options default_options( )
        {
            vtrc_rpc::session_options def_opts;
            return def_opts;
        }
    }

}}
