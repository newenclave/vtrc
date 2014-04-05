
#include <boost/asio.hpp>

#include "vtrc-application.h"
#include "vtrc-connection-list.h"

#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-pool-pair.h"

namespace vtrc { namespace server {

    struct application::impl {

        common::enviroment                  env_;
        boost::asio::io_service            *ios_;
        const bool                          own_ios_;

        boost::asio::io_service            *rpc_ios_;
        const bool                          own_rpc_ios_;

        vtrc::shared_ptr<common::connection_list>   clients_;

        impl( )
            :ios_(new boost::asio::io_service)
            ,own_ios_(true)
            ,rpc_ios_(new boost::asio::io_service)
            ,own_rpc_ios_(true)
            ,clients_(common::connection_list::create( ))
        { }

        impl( boost::asio::io_service *ios )
            :ios_(ios)
            ,own_ios_(false)
            ,rpc_ios_(ios)
            ,own_rpc_ios_(false)
            ,clients_(common::connection_list::create( ))
        { }

        impl( boost::asio::io_service *ios, boost::asio::io_service *rpc_ios )
            :ios_(ios)
            ,own_ios_(false)
            ,rpc_ios_(rpc_ios)
            ,own_rpc_ios_(false)
            ,clients_(common::connection_list::create( ))
        { }

        static
        bool close_all_connection( common::connection_iface_sptr next )
        {
            next->close( );
            return true;
        }

        void stop_all_clients( )
        {
            clients_->foreach_while( close_all_connection );
        }

        ~impl( ) try
        {
            stop_all_clients( );
            clients_->clear( );

            if( own_ios_ )      delete ios_;
            if( own_rpc_ios_ )  delete rpc_ios_;

        } catch ( ... ) {

        }

        common::enviroment &get_enviroment( )
        {
            return env_;
        }

        boost::asio::io_service &get_io_service( )
        {
            return *ios_;
        }

        boost::asio::io_service &get_rpc_service( )
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
    {}

    application::application(common::pool_pair &pools)
     :impl_(new impl(&pools.get_io_service( ), &pools.get_rpc_service( )))
    {

    }

    application::application( boost::asio::io_service &ios )
        :impl_(new impl(&ios))
    {}

    application::application( boost::asio::io_service &ios,
                           boost::asio::io_service &rpc_ios)
     :impl_(new impl(&ios, &rpc_ios))
    {}

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

    boost::asio::io_service &application::get_io_service( )
    {
        return impl_->get_io_service( );
    }

    boost::asio::io_service &application::get_rpc_service( )
    {
        return impl_->get_rpc_service( );
    }

    vtrc::shared_ptr<common::connection_list> application::get_clients()
    {
        return impl_->get_clients( );
    }

    common::rpc_service_wrapper_sptr application::get_service_by_name(
                                    common::connection_iface * /*connection*/,
                                    const std::string &        /*service_name*/)
    {
        return common::rpc_service_wrapper_sptr( );
    }

    std::string application::get_session_key(common::connection_iface *conn)
    {
        return std::string( );
    }

}}
