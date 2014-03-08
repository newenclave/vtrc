#include "vtrc-transformer-iface.h"
#include "vtrc-erseefor.h"

namespace vtrc { namespace common {

    namespace {

        static const size_t drop_key_init = 3072;

        struct transformer_erseefor: public transformer_iface {

            crypt::erseefor   crypt_;
            crypt::erseefor uncrypt_;

            transformer_erseefor( const char *transform_key, size_t t_length,
                                  const char *revert_key,    size_t r_length )
                :crypt_((const unsigned char *)transform_key, t_length)
                ,uncrypt_((const unsigned char *)revert_key,  r_length)
            {
                crypt_.drop( drop_key_init );
                uncrypt_.drop( drop_key_init );
            }

            void transform_data( char *data, size_t length )
            {
                crypt_.transform( data, data + length );
            }

            void revert_data( char *data, size_t length )
            {
                uncrypt_.transform( data, data + length );
            }
        };

    }

    namespace transformers { namespace erseefor {

        transformer_iface *create( const char *transform_key, size_t t_length,
                                   const char *revert_key,    size_t r_length)
        {
            return new transformer_erseefor( transform_key, t_length,
                                             revert_key,    r_length);
        }
    }}
}}

