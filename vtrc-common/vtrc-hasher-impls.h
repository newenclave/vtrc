#ifndef VTRC_HASHER_IMPLS_H
#define VTRC_HASHER_IMPLS_H

#include <string>

namespace vtrc { namespace common {

    struct hasher_iface;

namespace hasher {

    namespace  fake {
        hasher_iface *create( );
    }

    namespace sha2 {
        hasher_iface *create256( );
        hasher_iface *create384( );
        hasher_iface *create512( );
    }

    namespace crc {
        hasher_iface *create16( );
        hasher_iface *create32( );
    }
}

}}

#endif // VTRC_HASHER_IMPLS_H
