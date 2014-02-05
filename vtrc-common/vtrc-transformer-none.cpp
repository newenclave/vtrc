#include "vtrc-transformer-iface.h"

namespace vtrc { namespace common {

    namespace {
        struct transformer_none: public transformer_iface {
            void transform_data( char * /*data*/, size_t /*length*/ )
            {

            }
            void revert_data( char * /*data*/, size_t /*length*/ )
            {

            }
        };

    }

    namespace transformers { namespace none {
        transformer_iface *create( )
        {
            return new transformer_none;
        }
    }}
}}
