#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-custom.h"

#include "vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace server { namespace listeners {

    struct custom::impl {
        std::string           name_;
        bool                  active_;

        impl(const std::string &name)
            :name_(name)
            ,active_(false)
        { }
    };

    custom::custom( application & app,
            const rpc::session_options &opts,
            const std::string &name )
        :listener(app, opts)
        ,impl_(new impl(name))
    {

    }

    custom::~custom( )
    {

    }

    listener_sptr custom::create( application &app, const std::string &name )
    {
        rpc::session_options opts = common::defaults::session_options( );
        return create( app, opts, name );
    }

    listener_sptr custom::create( application &app,
                                  const rpc::session_options &opts,
                                  const std::string &name )
    {
        custom *inst = new custom( app, opts, name );
        return listener_sptr( inst );
    }

    std::string custom::name( ) const
    {
        return impl_->name_;
    }

    void custom::start( )
    {
        impl_->active_ = true;
    }

    void custom::stop( )
    {
        impl_->active_ = false;
    }

    bool custom::is_active( ) const
    {
        return impl_->active_;
    }

    bool custom::is_local( ) const
    {
        return true;
    }

}}}
