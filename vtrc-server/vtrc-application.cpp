
#include <boost/asio.hpp>

#include "vtrc-application.h"
#include "vtrc-common/vtrc-enviroment.h"

namespace vtrc { namespace server {

    struct application::application_impl {

        common::enviroment          env_;
        boost::asio::io_service    *ios_;
        const bool                  own_ios_;

        application_impl( )
            :ios_(new boost::asio::io_service)
            ,own_ios_(true)
        {}

        application_impl( boost::asio::io_service *ios )
            :ios_(ios)
            ,own_ios_(false)
        {}

        ~application_impl( )
        {
            if( own_ios_ ) delete ios_;
        }

        common::enviroment &get_enviroment( )
        {
            return env_;
        }

        boost::asio::io_service &get_io_service( )
        {
            return *ios_;
        }

    };

    application::application( )
        :impl_(new application_impl)
    {}

    application::application( boost::asio::io_service &ios )
        :impl_(new application_impl(&ios))
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

    }


}}
