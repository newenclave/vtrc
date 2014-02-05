#ifndef VTRC_TRANSFORMER_IFACE_H
#define VTRC_TRANSFORMER_IFACE_H

#include <stdlib.h>

namespace vtrc { namespace common {

    struct transformer_iface {
        virtual ~transformer_iface( ) { }
        virtual void transform_data( char *data, size_t length ) = 0;
        virtual void revert_data( char *data, size_t length ) = 0;
    };


    namespace transformers { namespace none {
        transformer_iface *create( );
    }}
}}

#endif // VTRC_TRANSFORMER_IFACE_H
