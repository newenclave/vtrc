#ifndef VTRC_CONNECTION_SETUP_IFACE_H
#define VTRC_CONNECTION_SETUP_IFACE_H

#include "vtrc-closure.h"

namespace vtrc { namespace rpc {

} }

namespace vtrc { namespace common {

    struct protocol_accessor;

    struct connection_setup_iface {
        virtual ~connection_setup_iface( ) { }
        virtual void init( protocol_accessor *pa ) = 0;
        virtual bool next( )             = 0;
        virtual bool error( )            = 0;
        virtual bool close_connection( ) = 0;
    };

}}

#endif // VTRC_CONNECTION_SETUP_IFACE_H
