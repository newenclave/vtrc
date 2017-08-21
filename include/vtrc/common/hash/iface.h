#ifndef VTRC_HASHER_IFACE_H
#define VTRC_HASHER_IFACE_H

#include <string>
#include "vtrc-memory.h"
#include "vtrc-stdint.h"

namespace vtrc { namespace common {

    namespace hash {

        struct iface {

            typedef vtrc::shared_ptr<iface> sptr;
            typedef vtrc::unique_ptr<iface> uptr;

            virtual ~iface( ) { }

            virtual size_t size( ) const = 0;

            virtual std::string get_data_hash( const void *data,
                                               size_t length) const = 0;

            virtual void get_data_hash(const void *data, size_t length,
                                             void *result_hash ) const = 0;

            virtual bool check_data_hash( const void *data, size_t length,
                                          const void *hash) const = 0;
        };

        namespace  none {
            iface *create( );
        }

        namespace sha2 {
            iface *create256( );
            iface *create384( );
            iface *create512( );
        }

        namespace crc {
            iface *create32( );
        }

        iface *create_by_index( unsigned var );
        iface *create_default( );

    }

}}

#endif // VTRC_HASHER_IFACE_H
