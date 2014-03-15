#include "vtrc-common-channel.h"

namespace vtrc { namespace common {

    namespace gpb = google::protobuf;

    struct common_channel::impl {

    };

    void common_channel::CallMethod(const gpb::MethodDescriptor* method,
                    gpb::RpcController* controller,
                    const gpb::Message* request,
                    gpb::Message* response,
                    gpb::Closure* done)
    {

    }

    common_channel::common_channel( )
        :impl_(new impl)
    {}

    common_channel::~common_channel( )
    {
        delete impl_;
    }

}}

