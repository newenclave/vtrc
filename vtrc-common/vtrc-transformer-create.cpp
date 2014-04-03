#include "vtrc-transformer-iface.h"
#include "protocol/vtrc-auth.pb.h"

namespace vtrc { namespace common {

    namespace transformers {
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

