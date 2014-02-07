
#include <memory.h>

#include "vtrc-hasher-iface.h"
#include "vtrc-hasher-impls.h"

#include "protocol/vtrc-auth.pb.h"

namespace vtrc { namespace common {  namespace hasher {

    namespace gpb = google::protobuf;

    hasher_iface *create_by_index( unsigned var )
    {
        hasher_iface *result = NULL;
        switch( var ) {
        case vtrc_auth::HASH_NONE:
            result = fake::create( );
            break;
        case vtrc_auth::HASH_CRC_16:
            result = crc::create16( );
            break;
        case vtrc_auth::HASH_CRC_32:
            result = crc::create32( );
            break;
        case vtrc_auth::HASH_CRC_64:
            result = crc::create64( );
            break;
        case vtrc_auth::HASH_SHA2_256:
            result = sha2::create256( );
            break;
        }
        return result;
    }

    hasher_iface *create_default( )
    {
        static const gpb::uint32 def_hasher =
                    vtrc_auth::client_selection::default_instance( ).hash( );
        return create_by_index( def_hasher );
    }

}}}
