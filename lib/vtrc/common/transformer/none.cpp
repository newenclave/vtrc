
#include "vtrc/common/transformer/iface.h"

namespace vtrc { namespace common {

    namespace {
        struct transformer_none: public transformer::iface {
            void transform( std::string & )
            {

            }
        };

    }

    namespace transformer { namespace none {
        transformer::iface *create( )
        {
            return new transformer_none;
        }
    }}
}}
