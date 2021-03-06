#include <algorithm>

#include "vtrc/common/transformer/iface.h"

#include "vtrc-auth.pb.h"

#include "vtrc/common/random-device.h"
#include "vtrc/common/hash/iface.h"

namespace vtrc { namespace common {

    namespace transformer {

        /**
          *   keys for basic authentication;
          *   result is: sha256( s2 + sha256( s1 + key ) );
          *   s1 and s2 are open information
         **/
        void create_key( const std::string &key,
                         const std::string &s1, const std::string &s2,
                               std::string &result )
        {
            std::string tkey(key);
            std::string ts1 (s1 );
            std::string ts2 (s2 );

            hash::iface::uptr sha256( hash::sha2::create256( ) );

            ts1.append( tkey.begin( ), tkey.end( ) );
            ts1.assign( sha256->get_data_hash( ts1.c_str( ), ts1.size( ) ) );

            ts2.append( ts1.begin( ), ts1.end( ) );
            ts2.assign( sha256->get_data_hash( ts2.c_str( ), ts2.size( ) ) );

            result.swap( ts2 );

        }

        /**
          *   Generate 2 blocks of open info and create secret key.
         **/
        void generate_key_infos( const std::string &key,
                                 std::string &s1, std::string &s2,
                                 std::string &result )
        {
            random_device rd( false );

            std::string ts1( rd.generate_block( 256 ) );
            std::string ts2( rd.generate_block( 256 ) );

            create_key( key, ts1, ts2, result );

            s1.swap( ts1 );
            s2.swap( ts2 );
        }

        transformer::iface *create( unsigned id, const char *key, size_t length)
        {
            transformer::iface *trans = VTRC_NULL;
            switch ( id ) {
            case rpc::auth::TRANSFORM_ERSEEFOR:
                trans = erseefor::create( key, length );
                break;
            case rpc::auth::TRANSFORM_NONE:
                trans = none::create( );
                break;
            default:
                break;
            }
            return trans;
        }
    }

}}

