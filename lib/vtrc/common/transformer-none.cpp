
#include "vtrc/common/transformer-iface.h"

namespace vtrc { namespace common {

    namespace {
        struct transformer_none: public transformer_iface {
            void transform( std::string & )
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
