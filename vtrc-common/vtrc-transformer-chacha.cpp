#include <iostream>
#include <algorithm>

#include "vtrc-transformer-iface.h"
#include "vtrc-random-device.h"
#include "vtrc-hash-iface.h"
#include "vtrc-memory.h"

#include "crypt/chacha/ecrypt-sync.h"

namespace vtrc { namespace common {

    namespace {

        struct transformer_chacha: public transformer_iface {

            ECRYPT_ctx ctx_;

            //// need poly!!!
            transformer_chacha( const char *transform_key, size_t t_length )
            {
                std::string key;
                key.reserve( 32 );
                size_t max = 32;

                static const size_t input_len = sizeof(ctx_.input) /
                                                sizeof(ctx_.input[0]);

                for( size_t i=0; i<input_len; ++i ) {
                    ctx_.input[i] = 0;
                }

                if( t_length < 32 ) {
                    while( max < 32 ) {
                        size_t next_len = std::min( 32 - max, t_length );
                        key.append( transform_key + max, next_len );
                        max += next_len;
                    }
                } else {
                    key.assign( transform_key, transform_key + 32 );
                }

//                std::cout << "Key is! "
//                          << transform_key << ":" << t_length
//                          << "\n";

                ECRYPT_keysetup( &ctx_,
                              reinterpret_cast<const uint8_t *>(key.c_str( )),
                              key.size( ) * 8, 0);
            }

            void transform( std::string &data )
            {
                char s;
                char *ptr = data.empty( ) ? &s : &data[0];
                ECRYPT_encrypt_bytes( &ctx_,
                                      reinterpret_cast<const uint8_t *>( ptr ),
                                      reinterpret_cast<      uint8_t *>( ptr ),
                                      data.size( ) );
            }

        };

    }

    namespace transformers { namespace chacha {
        transformer_iface *create( const char *transform_key, size_t length )
        {
            return new transformer_chacha( transform_key, length );
        }
    }}
}}


