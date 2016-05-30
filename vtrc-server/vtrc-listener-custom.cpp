#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-custom.h"
#include "vtrc-common/vtrc-connection-list.h"

#include "vtrc-protocol-layer-s.h"

#include "vtrc-rpc-lowlevel.pb.h"

#include "vtrc-bind.h"

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

    custom::shared_type custom::create( application &app,
                                        const std::string &name )
    {
        rpc::session_options opts = common::defaults::session_options( );
        return create( app, opts, name );
    }

    custom::shared_type custom::create( application &app,
                                        const rpc::session_options &opts,
                                        const std::string &name )
    {
        custom *inst = new custom( app, opts, name );
        return shared_type( inst );
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

    common::protocol_iface *custom::create_protocol(
            common::connection_iface_sptr conn )
    {
        vtrc::unique_ptr<protocol_layer_s> proto(
                    new protocol_layer_s( get_application( ), conn.get( ),
                                          get_options( ) ) );

        namespace ph = vtrc::placeholders;

        proto->set_precall( vtrc::bind( &custom::mk_precall, this,
                                ph::_1, ph::_2, ph::_3 ) );

        proto->set_postcall( vtrc::bind( &custom::mk_postcall, this,
                                   ph::_1, ph::_2 ) );

        proto->assign_lowlevel_factory( lowlevel_protocol_factory( ) );

        proto->init( );

        get_application( ).get_clients( )->store( conn );

        new_connection( conn.get( ) );

        try {
            conn->init( );
        } catch( ... ) {
            get_application( ).get_clients( )->drop( conn.get( ) );
            throw;
        }

        return proto.release( );
    }

    void custom::stop_client( common::connection_iface *con )
    {
        stop_connection( con );
        get_application( ).get_clients( )->drop( con );
    }

}}}
