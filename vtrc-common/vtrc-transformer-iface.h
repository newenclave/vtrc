#ifndef VTRC_TRANSFORMER_IFACE_H
#define VTRC_TRANSFORMER_IFACE_H

#include <stdlib.h>
#include "vtrc-memory.h"

namespace vtrc { namespace common {

    struct transformer_iface {
        virtual ~transformer_iface( ) { }
        virtual void transform( char *data, size_t length ) = 0;
    };

    typedef shared_ptr<transformer_iface> transformer_iface_sptr;

    namespace transformers {
        namespace none {
            transformer_iface *create( );
        }

        namespace erseefor {
            transformer_iface *create( const char *key, size_t t_length);

        }

//        transformer_iface *create_by_index( unsigned var );
//        transformer_iface *create_default( );

    }
}}

#endif // VTRC_TRANSFORMER_IFACE_H
