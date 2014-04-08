#include "vtrc-transformer-iface.h"
#include "vtrc-erseefor.h"
#include "vtrc-random-device.h"
#include "vtrc-hash-iface.h"
#include "vtrc-memory.h"

namespace vtrc { namespace common {

    namespace {

        static const size_t drop_key_init = 3072;

        struct transformer_erseefor: public transformer_iface {

            crypt::erseefor   crypt_;

            transformer_erseefor( const char *transform_key, size_t t_length )
                :crypt_((const unsigned char *)transform_key, t_length)
            {
                crypt_.drop( drop_key_init );
            }

            void transform( char *data, size_t length )
            {
                crypt_.transform( data, data + length );
            }
        };

    }

    namespace transformers { namespace erseefor {
        transformer_iface *create( const char *transform_key, size_t length )
        {
            return new transformer_erseefor( transform_key, length );
        }
    }}
}}

