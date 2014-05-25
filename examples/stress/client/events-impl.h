#ifndef EVENTS_IMPL_H
#define EVENTS_IMPL_H

#include "vtrc-memory.h"
#include "stress-iface.h"

namespace google { namespace protobuf {
    class Service;
}}

namespace vtrc { namespace client {
    class vtrc_client;
}}

namespace stress {
    google::protobuf::Service *create_events(
                               vtrc::shared_ptr<vtrc::client::vtrc_client> c );
}

#endif // EVENTS_IMPL_H
