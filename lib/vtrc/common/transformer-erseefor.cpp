#include <iostream>

#include "vtrc/common/erseefor.h"
#include "vtrc-memory.h"

#include "vtrc/common/random-device.h"
#include "vtrc/common/hash-iface.h"
#include "vtrc/common/transformer-iface.h"

namespace vtrc { namespace common {

    namespace {

        static const size_t drop_key_init = 3072;

        struct transformer_erseefor: public transformer_iface {

            crypt::erseefor   crypt_;

            transformer_erseefor( const char *transform_key, size_t t_length )
                :crypt_((const unsigned char *)transform_key, t_length)
            {
                crypt_.drop( drop_key_init );

//                std::cout << "RC4 Key is! "
//                          << transform_key << ":" << t_length
//                          << "\n";
            }

            void transform( std::string &data )
            {
                crypt_.transform( data.begin( ), data.end( ) );
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

