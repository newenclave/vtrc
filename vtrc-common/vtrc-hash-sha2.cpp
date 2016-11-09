#include <memory.h>

#include "vtrc-hash-iface.h"
#include "hash/sha2-traits.h"

namespace vtrc { namespace common {  namespace hash {

    namespace {

        template <typename HashTraits>
        struct hasher_sha: public hash_iface {

            typedef vtrc::uint8_t ubyte;

            size_t hash_size( ) const
            {
                return HashTraits::digest_length;
            }

            std::string get_data_hash( const void *data,
                                       size_t      length ) const
            {
                typename HashTraits::context_type context;
                HashTraits::init( &context );
                HashTraits::update( &context,
                    reinterpret_cast<const ubyte *>(data), length );

                //return HashTraits::fin( &context );

                typename HashTraits::digest_block_type res;
                HashTraits::fin( &context, res );
                return std::string( &res[0], &res[HashTraits::digest_length] );
            }

            void get_data_hash( const void *data, size_t length,
                                      void *result_hash ) const
            {
                typename HashTraits::context_type context;
                HashTraits::init( &context );
                HashTraits::update( &context,
                    reinterpret_cast<const ubyte *>(data), length );

                HashTraits::fin( &context,
                                 reinterpret_cast<ubyte *>( result_hash ) );
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
#if VTRC_OPENSSL_ENABLED
        hash_iface *create256( )
        {
            return new hasher_sha<vtrc::hash::hash_SHA256_traits>;
        }

        hash_iface *create348( )
        {
            return new hasher_sha<vtrc::hash::hash_SHA384_traits>;
        }

        hash_iface *create512( )
        {
            return new hasher_sha<vtrc::hash::hash_SHA512_traits>;
        }
#else
		hash_iface *create256( )
		{
			return new hasher_sha<vtrc::hash::hash_SHA256i_traits>;
		}

		hash_iface *create348( )
		{
			return new hasher_sha<vtrc::hash::hash_SHA384i_traits>;
		}

		hash_iface *create512( )
		{
			return new hasher_sha<vtrc::hash::hash_SHA512i_traits>;
		}
#endif
    }

}}}
