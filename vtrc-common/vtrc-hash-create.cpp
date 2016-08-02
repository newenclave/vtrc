#include <algorithm>
#include <memory.h>

#include "vtrc-hash-iface.h"
#include "vtrc-stdint.h"
#include "vtrc-auth.pb.h"

namespace vtrc { namespace common {  namespace hash {

    namespace gpb = google::protobuf;

    hash_iface *create_by_index( unsigned var )
    {
        namespace auth = vtrc::rpc::auth;
        hash_iface *result = NULL;
        switch( var ) {
        case auth::HASH_NONE:
            result = none::create( );
            break;
        case auth::HASH_CRC_16:
            result = crc::create16( );
            break;
        case auth::HASH_CRC_32:
            result = crc::create32( );
            break;
//        case auth::HASH_CRC_64:
//            result = crc::create64( );
            break;
        case auth::HASH_SHA2_256:
            result = sha2::create256( );
            break;
        }
        return result;
    }

    hash_iface *create_default( )
    {
        static const unsigned def_hasher =
                vtrc::rpc::auth::client_selection::default_instance( ).hash( );
        return create_by_index( def_hasher );
    }

}}}
