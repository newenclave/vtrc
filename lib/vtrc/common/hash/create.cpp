#include <algorithm>
#include <memory.h>

#include "vtrc-stdint.h"
#include "vtrc-cxx11.h"
#include "vtrc-auth.pb.h"

#include "vtrc/common/hash/iface.h"

namespace vtrc { namespace common {  namespace hash {

    namespace gpb = google::protobuf;

    iface *create_by_index( unsigned var )
    {
        namespace auth = vtrc::rpc::auth;
        iface *result = VTRC_NULL;
        switch( var ) {
        case auth::HASH_NONE:
            result = none::create( );
            break;
        case auth::HASH_CRC_32:
            result = crc::create32( );
            break;
        case auth::HASH_SHA2_256:
            result = sha2::create256( );
            break;
        }
        return result;
    }

    iface *create_default( )
    {
        static const unsigned def_hasher =
                vtrc::rpc::auth::client_selection::default_instance( ).hash( );
        return create_by_index( def_hasher );
    }

}}}
