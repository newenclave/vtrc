#include "vtrc-transformer-iface.h"
#include "protocol/vtrc-auth.pb.h"

#include "vtrc-random-device.h"
#include "vtrc-hash-iface.h"

namespace vtrc { namespace common {

    namespace transformers {

        void create_key( const std::string &key,
                         const std::string &s1, const std::string &s2,
                               std::string &result )
        {
            std::string tmpkey(key);

            std::string tmps1(s1);
            std::string tmps2(s2);

            vtrc::scoped_ptr<hash_iface> sha256(hash::sha2::create256( ));

            tmps1.append( tmpkey.begin( ), tmpkey.end( ) );
            tmps1.assign( sha256->get_data_hash(tmps1.c_str( ), tmps1.size( )));

            tmps2.append( tmps1.begin( ), tmps1.end( ) );
            tmps2.assign( sha256->get_data_hash(tmps2.c_str( ), tmps2.size( )));

            result.swap( tmps2 );

        }

        void generate_key_infos( const std::string &key,
                             std::string &s1, std::string &s2,
                             std::string &result )
        {
            random_device rd( false );

            std::string tmps1(rd.generate_block( 256 ));
            std::string tmps2(rd.generate_block( 256 ));

            create_key( key, tmps1, tmps2, result );

            s1.swap( tmps1 );
            s2.swap( tmps2 );
        }

        transformer_iface *create( unsigned id,
                                   const char *key, size_t length)
        {
            transformer_iface *trans = NULL;
            switch ( id ) {
            case vtrc_auth::TRANSFORM_ERSEEFOR:
                trans = erseefor::create( key, length );
                break;
            case vtrc_auth::TRANSFORM_NONE:
                trans = none::create( );
                break;
            default:
                break;
            }
            return trans;
        }
    }

}}

