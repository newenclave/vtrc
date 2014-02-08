#ifndef VTRC_TRANSFORMER_IFACE_H
#define VTRC_TRANSFORMER_IFACE_H

#include <stdlib.h>
#include <boost/shared_ptr.hpp>

namespace vtrc { namespace common {

    struct transformer_iface {
        virtual ~transformer_iface( ) { }
        virtual void transform_data( char *data, size_t length ) = 0;
        virtual void revert_data( char *data, size_t length ) = 0;
    };

    typedef boost::shared_ptr<transformer_iface> transformer_iface_sptr;

    namespace transformers {
        namespace none {
            transformer_iface *create( );
        }

        namespace erseefor {
            transformer_iface *create( const char *key, size_t keysize );
        }

//        transformer_iface *create_by_index( unsigned var );
//        transformer_iface *create_default( );

    }
}}

#endif // VTRC_TRANSFORMER_IFACE_H
