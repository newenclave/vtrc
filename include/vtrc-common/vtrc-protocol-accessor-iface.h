#ifndef VTRCP_ROTOCOL_ACCESSOR_IFACE_H
#define VTRCP_ROTOCOL_ACCESSOR_IFACE_H

#include "vtrc-closure.h"

namespace vtrc { namespace rpc {

}}

namespace vtrc { namespace common {

    struct transformer_iface;
    struct hash_iface;

    struct protocol_accessor {
        virtual ~protocol_accessor( ) { }
        virtual void set_transformer( transformer_iface *ti ) = 0;
        virtual void set_revertor( transformer_iface *ti ) = 0;
        virtual void set_hash_maker( hash_iface *hi ) = 0;
        virtual void set_hash_cheker( hash_iface *hi ) = 0;
        virtual void write( const std::string &data,
                            system_closure_type cb, bool on_send_success ) = 0;
    };

}}

#endif // VTRCP_ROTOCOL_ACCESSOR_IFACE_H
