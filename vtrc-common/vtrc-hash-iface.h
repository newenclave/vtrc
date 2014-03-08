#ifndef VTRC_HASHER_IFACE_H
#define VTRC_HASHER_IFACE_H

#include <string>
#include "vtrc-memory.h"

namespace vtrc { namespace common {
    struct hash_iface {
        virtual ~hash_iface( ) { }

        virtual size_t hash_size( ) const = 0;

        virtual std::string get_data_hash(const void *data,
                                          size_t length) const = 0;

        virtual bool check_data_hash( const void *data, size_t length,
                                      const void *hash) const = 0;
    };

    typedef shared_ptr<hash_iface> hasher_iface_sptr;

    namespace hash {

        namespace  none {
            hash_iface *create( );
        }

        namespace sha2 {
            hash_iface *create256( );
            hash_iface *create384( );
            hash_iface *create512( );
        }

        namespace crc {
            hash_iface *create16( );
            hash_iface *create32( );
            hash_iface *create64( );
        }

        hash_iface *create_by_index( unsigned var );
        hash_iface *create_default( );

    }

}}

#endif // VTRC_HASHER_IFACE_H
