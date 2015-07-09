#ifndef VTRCP_ROTOCOL_ACCESSOR_IFACE_H
#define VTRCP_ROTOCOL_ACCESSOR_IFACE_H

#include "vtrc-closure.h"

namespace vtrc { namespace common {
    struct connection_iface;
}}

namespace vtrc { namespace rpc {
    class session_options;
}}

namespace vtrc { namespace common {

    struct transformer_iface;
    struct hash_iface;

    struct protocol_accessor {

        virtual ~protocol_accessor( ) { }

        virtual void set_transformer( transformer_iface *ti ) = 0;
        virtual void set_revertor( transformer_iface *ti ) = 0;
        virtual void set_hash_maker( hash_iface *hi ) = 0;
        virtual void set_hash_checker( hash_iface *hi ) = 0;

        virtual void set_client_id( const std::string &id ) = 0;

        virtual void write( const std::string &data,
                            system_closure_type cb, bool on_send_success ) = 0;
        virtual common::connection_iface *connection( ) = 0;

        /// for client-side only
        virtual void configure_session( const vtrc::rpc::session_options & )
        { }

        virtual void close( ) = 0;
    };

    /// vtrc-common/vtrc-protocol-layer.cpp
    vtrc::rpc::session_options default_session_options( );
}}

#endif // VTRCP_ROTOCOL_ACCESSOR_IFACE_H
