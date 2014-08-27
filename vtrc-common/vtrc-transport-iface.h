#ifndef VTRC_TRANSPORT_IFACE_H
#define VTRC_TRANSPORT_IFACE_H

#include <stdlib.h>

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-closure.h"

namespace boost {

    namespace system {
        class error_code;
    }
}

namespace vtrc { namespace common {


    struct transport_iface: public connection_iface {

        virtual ~transport_iface( ) { }

        virtual std::string name( ) const   = 0;
        virtual void close( )               = 0;
        virtual bool active( ) const        = 0;
        virtual void init( )                = 0;

        virtual void write( const char *data, size_t length ) = 0;

        virtual void write( const char *data, size_t length,
                                        const system_closure_type &success,
                                        bool success_on_send ) = 0;

        virtual void on_close( ) { }

    };

}}

#endif // VTRC_TRANSPORT_IFACE_H
