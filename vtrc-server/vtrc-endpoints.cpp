
#include "vtrc-endpoint-base.h"

namespace vtrc { namespace server { namespace endpoints {

    endpoint_options default_options( )
    {
        endpoint_options def_opts = { 5, 1024 * 1024, 20, 4096 };
        return def_opts;
    }

}}}

