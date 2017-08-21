
#include <memory.h>

#include "vtrc/common/hash/iface.h"
#include "vtrc/common/hash/crc32-impl.h"

namespace vtrc { namespace common {  namespace hash {

    namespace {
        struct hasher_crc32: public iface {
            size_t size( ) const
            {
                return crc32_impl::length( );
            }

            std::string get_data_hash(const void *data, size_t length) const
            {
                std::string res( size( ), 0 );
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
        iface *create32( )
        {
            return new hasher_crc32;
        }
    }


}}}
