#ifndef VTRC_SYSTEM_H
#define VTRC_SYSTEM_H

#if defined(ASIO_STANDALONE)
#include "asio/error_code.hpp"
namespace vtrc {
    inline
    const asio::error_category &get_system_category( )
    {
        return asio::system_category( );
    }
}
#else
#include "boost/system/error_code.hpp"
namespace vtrc {
    using boost::system::get_system_category;
}
#endif
#include "vtrc-system-forward.h"


#endif // VTRCSYSTEM_H

