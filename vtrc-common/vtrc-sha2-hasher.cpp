#include <memory.h>

#include "vtrc-hasher-iface.h"
#include "hash/sha2-traits.h"

namespace vtrc { namespace common {  namespace hasher {

    namespace {

        template <typename HashTraits>
        struct hasher_sha: public hasher_iface {

            size_t hash_size( ) const
            {
                return HashTraits::digest_length;
            }

            std::string get_data_hash(const void *data,
                                      size_t      length ) const
            {
                typename HashTraits::context_type context;
                HashTraits::init( &context );
                HashTraits::update( &context,
                    reinterpret_cast<const u_int8_t *>(data), length );

                return HashTraits::final( &context );
            }

            bool check_data_hash( const void *  data  ,
                                  size_t        length,
                                  const void *  hash  ) const
            {
                std::string h = get_data_hash( data, length );
                return memcmp( h.c_str( ), hash,
                               HashTraits::digest_length ) == 0;
            }

        };
    }

    namespace sha2 {
        hasher_iface *create256( )
        {
            return new hasher_sha<vtrc::hash::hash_SHA256_traits>;
        }

        hasher_iface *create348( )
        {
            return new hasher_sha<vtrc::hash::hash_SHA384_traits>;
        }

        hasher_iface *create512( )
        {
            return new hasher_sha<vtrc::hash::hash_SHA512_traits>;
        }
    }

}}}
