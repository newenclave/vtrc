
#include <map>
#include <google/protobuf/descriptor.h>

#include "vtrc-rpc-channel.h"
#include "vtrc-memory.h"
#include "vtrc-mutex.h"
#include "vtrc-mutex-typedefs.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "proto-helper/message-utilities.h"

namespace vtrc { namespace common  {

    namespace {

    }

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

