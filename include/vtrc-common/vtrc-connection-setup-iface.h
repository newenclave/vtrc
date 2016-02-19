#ifndef VTRC_CONNECTION_SETUP_IFACE_H
#define VTRC_CONNECTION_SETUP_IFACE_H

#include <string>
#include "vtrc-closure.h"

namespace vtrc { namespace common {

    struct protocol_accessor;

    struct connection_setup_iface {
        virtual ~connection_setup_iface( ) { }
        virtual void init( protocol_accessor *pa,
                           system_closure_type ready_cb ) = 0;
        virtual bool do_handshake( const std::string &data ) = 0;
        virtual bool ready( ) const = 0;
    };

}}

#endif // VTRC_CONNECTION_SETUP_IFACE_H
