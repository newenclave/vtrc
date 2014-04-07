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

        void create_session_key( const std::string &key,
                             std::string &result,
                             std::string &s1,
                             std::string &s2 )
        {
            random_device rd( false );
            std::string tmpkey(key);

            std::string tmps1(rd.generate_block( 256 ));
            std::string tmps2(rd.generate_block( 256 ));

            std::string ready_s1(tmps1);
            std::string ready_s2(tmps2);

            vtrc::scoped_ptr<hash_iface> sha256(hash::sha2::create256( ));

            tmps1.append( tmpkey.begin( ), tmpkey.end( ) );
            tmps1.assign( sha256->get_data_hash(tmps1.c_str( ), tmps1.size( )));

            tmps2.append( tmps1.begin( ), tmps1.end( ) );
            tmps2.assign( sha256->get_data_hash(tmps2.c_str( ), tmps2.size( )));

            result.swap( tmps2 );
            s1.swap( ready_s1 );
            s2.swap( ready_s2 );
        }

        transformer_iface *create( const char *transform_key, size_t t_length )
        {
            return new transformer_erseefor( transform_key, t_length );
        }
    }}
}}

