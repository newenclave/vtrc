#include "vtrc-rpc-channel.h"

namespace vtrc { namespace common  {

    struct rpc_channel::impl {

    };

    rpc_channel::rpc_channel( )
        :impl_(new impl)
    {}

    rpc_channel::~rpc_channel( )
    {
        delete impl_;
    }

}}

