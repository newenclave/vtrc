
#include "vtrc/server/listener.h"
#include "vtrc/server/application.h"

#include "vtrc/common/environment.h"
#include "vtrc-atomic.h"
#include "vtrc-bind.h"
#include "vtrc-cxx11.h"

#include "vtrc-rpc-lowlevel.pb.h"


namespace vtrc { namespace server {

    namespace {
        namespace  gpb = google::protobuf;
    }

    struct listener::impl {
        application              &app_;
        common::environment        env_;
        rpc::session_options      opts_;

        listener                 *parent_;
        vtrc::atomic<size_t>      client_count_;

        listener::lowlevel_factory_type lowlevel_factory_;

        impl( application &app, const rpc::session_options &opts )
            :app_(app)
            ,env_(app_.get_enviroment( ))
            ,opts_(opts)
            ,parent_(VTRC_NULL)
            ,client_count_(0)
        { }
    };

    listener::listener( application & app, const rpc::session_options &opts )
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

    common::environment  &listener::get_enviroment( )
    {
        return impl_->env_;
    }

    const rpc::session_options &listener::get_options( ) const
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

    void listener::new_connection( common::connection_iface *conn )
    {
        ++impl_->client_count_;
        on_new_connection_( conn );
    }

    void listener::stop_connection( common::connection_iface *conn )
    {
        --impl_->client_count_;
        on_stop_connection_( conn );
    }

    void listener::call_on_accept_failed( const VTRC_SYSTEM::error_code &err )
    {
        on_accept_failed_( err );
    }

    void listener::call_on_stop( )
    {
        on_stop_( );
    }

    void listener::call_on_start( )
    {
        on_start_( );
    }

    void listener::assign_lowlevel_protocol_factory(
            listener::lowlevel_factory_type factory )
    {
        impl_->lowlevel_factory_ = factory;
    }

    listener::lowlevel_factory_type listener::lowlevel_protocol_factory( )
    {
        return impl_->lowlevel_factory_;
    }

}}
