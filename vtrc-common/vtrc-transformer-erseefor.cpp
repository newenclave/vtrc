#include "vtrc-transformer-iface.h"
#include "vtrc-erseefor.h"

namespace vtrc { namespace common {

    namespace {

        static const size_t drop_key_init = 3072;

        struct transformer_erseefor: public transformer_iface {

            crypt::erseefor   crypt_;
            crypt::erseefor uncrypt_;

            transformer_erseefor( const char *key, size_t length )
                :crypt_((const unsigned char *)key, length)
                ,uncrypt_((const unsigned char *)key, length)
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

        transformer_iface *create( const char *key, size_t length )
        {
            return new transformer_erseefor( key, length );
        }
    }}
}}

