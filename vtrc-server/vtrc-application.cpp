
#include <boost/asio.hpp>

#include "vtrc-application.h"
#include "vtrc-connection-list.h"

#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"

namespace vtrc { namespace server {

    struct application::impl {

        common::enviroment                  env_;
        boost::asio::io_service            *ios_;
        const bool                          own_ios_;

        shared_ptr<connection_list>         clients_;

        impl( )
            :ios_(new boost::asio::io_service)
            ,own_ios_(true)
            ,clients_(make_shared<connection_list>( ))
        { }

        impl( boost::asio::io_service *ios )
            :ios_(ios)
            ,own_ios_(false)
            ,clients_(make_shared<connection_list>( ))
        { }

        ~impl( ) try
        {
            clients_->clear( );
            if( own_ios_ ) delete ios_;

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

        shared_ptr<connection_list> get_clients( )
        {
            return clients_;
        }

    };

    application::application( )
        :impl_(new impl)
    {}

    application::application( boost::asio::io_service &ios )
        :impl_(new impl(&ios))
    {}

    application::~application( )
    {
        delete impl_;
    }

    common::enviroment &application::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

    boost::asio::io_service &application::get_io_service( )
    {
        return impl_->get_io_service( );
    }

    shared_ptr<connection_list> application::get_clients()
    {
        return impl_->get_clients( );
    }

    common::rpc_service_wrapper_sptr application::get_service_by_name(
                                    common::connection_iface * /*connection*/,
                                    const std::string &        /*service_name*/)
    {
        return common::rpc_service_wrapper_sptr( );
    }

}}
