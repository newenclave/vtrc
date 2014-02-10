
#include "vtrc-application.h"
#include "vtrc-common/vtrc-enviroment.h"

namespace vtrc { namespace server {

    struct application::application_impl {

        common::enviroment env_;
        common::enviroment &get_enviroment( )
        {
            return env_;
        }

    };

    application::application( )
        :impl_(new application_impl)
    {

    }

    application::~application( )
    {
        delete impl_;
    }

    common::enviroment &application::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

}}
