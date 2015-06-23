#include "vtrc-listener.h"
#include "vtrc-application.h"

#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-atomic.h"
#include "vtrc-bind.h"

#include "vtrc-rpc-lowlevel.pb.h"


namespace vtrc { namespace server {

    namespace {

        namespace  gpb = google::protobuf;

        common::precall_closure_type empty_pre( )
        {
            struct call_type {
                static void call( common::connection_iface &,
                                  const gpb::MethodDescriptor *,
                                  rpc::lowlevel_unit & )
                { }
            };
            namespace ph = vtrc::placeholders;
            return vtrc::bind( &call_type::call, ph::_1, ph::_2, ph::_3 );
        }

        common::postcall_closure_type empty_post( )
        {
            struct call_type {
                static void call( common::connection_iface &,
                                  rpc::lowlevel_unit & )
                { }
            };
            namespace ph = vtrc::placeholders;
            return vtrc::bind( &call_type::call, ph::_1, ph::_2 );
        }
    }

    struct listener::impl {
        application              &app_;
        common::enviroment        env_;
        rpc::session_options      opts_;

        listener                 *parent_;
        vtrc::atomic<size_t>      client_count_;

        common::precall_closure_type  precall_;
        common::postcall_closure_type postcall_;

        impl( application &app, const rpc::session_options &opts )
            :app_(app)
            ,env_(app_.get_enviroment( ))
            ,opts_(opts)
            ,client_count_(0)
            ,precall_(empty_pre( ))
            ,postcall_(empty_post( ))
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

    common::enviroment  &listener::get_enviroment( )
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

    void listener::mk_precall( common::connection_iface &connection,
                               const google::protobuf::MethodDescriptor *method,
                               rpc::lowlevel_unit &llu )
    {
        impl_->precall_( connection, method, llu );
    }

    void listener::mk_postcall( common::connection_iface &connection,
                                rpc::lowlevel_unit &llu )
    {
        impl_->postcall_( connection, llu );
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

    void listener::set_precall( const common::precall_closure_type &func )
    {
        impl_->precall_ = func;
    }

    void listener::set_postcall( const common::postcall_closure_type &func )
    {
        impl_->postcall_ = func;
    }

    namespace listeners {
        rpc::session_options default_options( )
        {
            rpc::session_options res;
            res.set_max_active_calls  ( 5 );
            res.set_max_message_length( 65536 );
            res.set_max_stack_size    ( 64 );
            res.set_read_buffer_size  ( 4096 );
            return res;
        }
    }

}}
