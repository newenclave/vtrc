
#include <memory.h>

#include "vtrc/common/hash-iface.h"
#include "hash/crc32-impl.h"

namespace vtrc { namespace common {  namespace hash {

    namespace {
        struct hasher_crc32: public hash_iface {
            size_t hash_size( ) const
            {
                return crc32_impl::length( );
            }

            std::string get_data_hash(const void *data, size_t length) const
            {
                std::string res( hash_size( ), 0 );
                crc32_impl::get( reinterpret_cast<const char *>(data), length,
                                 &res[0] );
                return res;
            }

            void get_data_hash( const void *data, size_t length,
                                void *result_hash ) const
            {
                crc32_impl::get( reinterpret_cast<const char *>(data), length,
                                 reinterpret_cast<char *>(result_hash) );
            }

            bool check_data_hash( const void *data, size_t length,
                                  const void *hash) const
            {
                return crc32_impl::check( reinterpret_cast<const char *>(data),
                                    length,
                                    reinterpret_cast<const char *>(hash ) );
            }
        };
    }

    namespace crc {
        hash_iface *create32( )
        {
            return new hasher_crc32;
        }
    }


}}}
