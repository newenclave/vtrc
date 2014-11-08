#ifndef SHA2_TRAITS_H
#define SHA2_TRAITS_H

#include <string>
#include "sha2.h"

#include "vtrc-stdint.h"

namespace vtrc { namespace hash {

#define HASH_TRAIT_DEFINE( NAME )                                              \
    struct hash_##NAME##_traits {                                              \
                                                                               \
        static const size_t digest_length        = NAME##_DIGEST_LENGTH;       \
        static const size_t digest_string_length = NAME##_DIGEST_STRING_LENGTH;\
                                                                               \
        typedef vtrc::uint8_t  digest_block_type[digest_length];                     \
        typedef char           digest_string_type[digest_string_length];             \
                                                                               \
        typedef NAME##_CTX  context_type;                                      \
                                                                               \
        static void init( context_type *context )                              \
        {                                                                      \
            NAME##_Init(context);                                              \
        }                                                                      \
                                                                               \
        static                                                                 \
        void update( context_type *context,                                    \
                     const vtrc::uint8_t *data, size_t len )                   \
        {                                                                      \
            NAME##_Update( context, data, len );                               \
        }                                                                      \
                                                                               \
        static std::string fin( context_type *context )                        \
        {                                                                      \
            digest_block_type data;                                            \
            NAME##_Final( data, context );                                     \
            return std::string( (const char *)&data[0],                        \
                                (const char *)&data[digest_length] );          \
        }                                                                      \
                                                                               \
        static void fin( context_type *context, vtrc::uint8_t *result )        \
        {                                                                      \
            NAME##_Final( result, context );                                   \
        }                                                                      \
                                                                               \
        static std::string end( context_type *context )                        \
        {                                                                      \
            digest_string_type result;                                         \
            NAME##_End( context, result );                                     \
                                                                               \
            return std::string( &result[0] );                                  \
        }                                                                      \
                                                                               \
        static std::string data( const vtrc::uint8_t *data, size_t length )    \
        {                                                                      \
            digest_string_type result;                                         \
            NAME##_Data( data, length, result );                               \
            return std::string( (const char *)&result[0] );                    \
        }                                                                      \
    }

    HASH_TRAIT_DEFINE( SHA256 );
    HASH_TRAIT_DEFINE( SHA384 );
    HASH_TRAIT_DEFINE( SHA512 );

#undef HASH_TRAIT_DEFINE

}}

#endif // SHA2TRAITS_H
