#ifndef VTRC_TRANSPORT_IFACE_H
#define VTRC_TRANSPORT_IFACE_H

#include <stdlib.h>

#include "vtrc-connection-iface.h"

namespace boost {

    namespace system {
        class error_code;
    }
}

namespace vtrc { namespace common {

    struct transport_iface: public connection_iface {

        virtual ~transport_iface( ) { }
        //
        virtual void init( ) = 0;
        virtual void write( const char *data, size_t length ) = 0;
        virtual void write( const char *data, size_t length,
                                      closure_type &success ) = 0;
        virtual void write_raw( const char *data, size_t length ) = 0;
    };

}}

#endif // VTRC_TRANSPORT_IFACE_H
