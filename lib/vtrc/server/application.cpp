
#include "vtrc-asio.h"

#include "vtrc/server/application.h"

#include "vtrc/common/connection-list.h"

#include "vtrc/common/environment.h"
#include "vtrc/common/rpc-service-wrapper.h"
#include "vtrc/common/pool-pair.h"

namespace vtrc { namespace server {

    namespace {
        namespace gpb = google::protobuf;
        static
        vtrc::shared_ptr<gpb::Service>
        default_factory( common::connection_iface *, const std::string & )
        {
            return vtrc::shared_ptr<gpb::Service>( );
        }

        typedef common::protocol_iface::call_type call_type;
    }

    struct application::impl {

        common::environment        env_;
        VTRC_ASIO::io_service     *ios_;
        VTRC_ASIO::io_service     *rpc_ios_;


        common::connection_list::sptr clients_;

        service_factory_type       factory_;

        const bool                 own_ios_;
        const bool                 own_rpc_ios_;

        impl( const impl & );
        impl & operator = ( const impl & );

        impl( )
            :ios_(new VTRC_ASIO::io_service)
            ,rpc_ios_(ios_)
            ,clients_(common::connection_list::create( ))
            ,factory_(&default_factory)
            ,own_ios_(true)
            ,own_rpc_ios_(false)
        { }

        impl( VTRC_ASIO::io_service *ios )
            :ios_(ios)
            ,rpc_ios_(ios)
            ,clients_(common::connection_list::create( ))
            ,factory_(&default_factory)
            ,own_ios_(false)
            ,own_rpc_ios_(false)
        { }

        impl( VTRC_ASIO::io_service *ios, VTRC_ASIO::io_service *rpc_ios )
            :ios_(ios)
            ,rpc_ios_(rpc_ios)
            ,clients_(common::connection_list::create( ))
            ,factory_(&default_factory)
            ,own_ios_(false)
            ,own_rpc_ios_(false)
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

            if( own_ios_ )     { delete ios_;     }
            if( own_rpc_ios_ ) { delete rpc_ios_; }

        } catch ( ... ) {
            ;;;
        } }

        common::environment &get_enviroment( )
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

        common::connection_list::sptr get_clients( )
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

    common::environment &application::get_enviroment( )
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

    const common::environment &application::get_enviroment( )  const
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

    void application::execute( common::protocol_iface::call_type call )
    {
        impl_->rpc_ios_->post( call );
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
        vtrc::shared_ptr<gpb::Service> svc =
                     impl_->factory_( connection, service_name );
        return svc ? vtrc::make_shared<common::rpc_service_wrapper>( svc )
                   : common::rpc_service_wrapper_sptr( );
    }

    std::string application::get_session_key(common::connection_iface * /*c*/,
                                             const std::string & /*id*/ )
    {
        return std::string( );
    }

    bool application::session_key_required( common::connection_iface * /*conn*/,
                                            const std::string & /*id*/)
    {
        return false;
    }

}}
