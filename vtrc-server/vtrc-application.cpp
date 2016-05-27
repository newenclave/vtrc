
#include "vtrc-asio.h"

#include "vtrc-application.h"

#include "vtrc-common/vtrc-connection-list.h"

#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-pool-pair.h"

namespace vtrc { namespace server {

    namespace {
        namespace gpb = google::protobuf;
        static
        vtrc::shared_ptr<gpb::Service>
        default_factory( common::connection_iface *, const std::string & )
        {
            return vtrc::shared_ptr<gpb::Service>( );
        }
    }

    struct application::impl {

        common::enviroment         env_;
        VTRC_ASIO::io_service     *ios_;
        const bool                 own_ios_;

        VTRC_ASIO::io_service     *rpc_ios_;
        const bool                 own_rpc_ios_;

        vtrc::shared_ptr<common::connection_list> clients_;

        service_factory_type        factory_;

        impl( )
            :ios_(new VTRC_ASIO::io_service)
            ,own_ios_(true)
            ,rpc_ios_(ios_)
            ,own_rpc_ios_(false)
            ,clients_(common::connection_list::create( ))
            ,factory_(&default_factory)
        { }

        impl( VTRC_ASIO::io_service *ios )
            :ios_(ios)
            ,own_ios_(false)
            ,rpc_ios_(ios)
            ,own_rpc_ios_(false)
            ,clients_(common::connection_list::create( ))
            ,factory_(&default_factory)
        { }

        impl( VTRC_ASIO::io_service *ios, VTRC_ASIO::io_service *rpc_ios )
            :ios_(ios)
            ,own_ios_(false)
            ,rpc_ios_(rpc_ios)
            ,own_rpc_ios_(false)
            ,clients_(common::connection_list::create( ))
            ,factory_(&default_factory)
        { }

        static bool close_all_connection( common::connection_iface_sptr next )
        {
            next->close( );
            return true;
        }

        void stop_all_clients( )
        {
            clients_->foreach_while( close_all_connection );
        }

        ~impl( )
        { try {

            stop_all_clients( );
            clients_->clear( );

            if( own_ios_ )     delete ios_;
            if( own_rpc_ios_ ) delete rpc_ios_;

        } catch ( ... ) {
            ;;;
        } }

        common::enviroment &get_enviroment( )
        {
            return env_;
        }

        VTRC_ASIO::io_service &get_io_service( )
        {
            return *ios_;
        }

        VTRC_ASIO::io_service &get_rpc_service( )
        {
            return *rpc_ios_;
        }

        vtrc::shared_ptr<common::connection_list> get_clients( )
        {
            return clients_;
        }

    };

    application::application(  )
        :impl_(new impl)
    { }

    application::application(common::pool_pair &pools)
        :impl_(new impl(&pools.get_io_service( ), &pools.get_rpc_service( )))
    {

    }

    application::application( VTRC_ASIO::io_service &ios )
        :impl_(new impl(&ios))
    { }

    application::application( VTRC_ASIO::io_service &ios,
                              VTRC_ASIO::io_service &rpc_ios)
        :impl_(new impl(&ios, &rpc_ios))
    { }

    application::~application( )
    {
        delete impl_;
    }

    void application::stop_all_clients( )
    {
        impl_->stop_all_clients( );
    }

    common::enviroment &application::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

    VTRC_ASIO::io_service &application::get_io_service( )
    {
        return impl_->get_io_service( );
    }

    VTRC_ASIO::io_service &application::get_rpc_service( )
    {
        return impl_->get_rpc_service( );
    }

    const common::enviroment &application::get_enviroment( )  const
    {
        return impl_->get_enviroment( );
    }

    const VTRC_ASIO::io_service &application::get_io_service( )  const
    {
        return impl_->get_io_service( );
    }

    const VTRC_ASIO::io_service &application::get_rpc_service( ) const
    {
        return impl_->get_rpc_service( );
    }

    vtrc::shared_ptr<common::connection_list> application::get_clients()
    {
        return impl_->get_clients( );
    }

    void application::assign_service_factory( service_factory_type factory )
    {
        impl_->factory_ = factory ? factory : &default_factory;
    }

    void application::configure_session( common::connection_iface *  /*c*/,
                                         rpc::session_options & /*opts*/ )
    {
        /// use default settings
    }

    common::rpc_service_wrapper_sptr application::get_service_by_name(
                                    common::connection_iface * connection,
                                    const std::string &        service_name)
    {
        common::rpc_service_wrapper_sptr res =
                vtrc::make_shared<common::rpc_service_wrapper>
                    (impl_->factory_( connection, service_name ) );
        return res;
    }

    std::string application::get_session_key(common::connection_iface *conn,
                                             const std::string &id)
    {
        return std::string( );
    }

    bool application::session_key_required( common::connection_iface* conn,
                                            const std::string &id)
    {
        return false;
    }

}}
