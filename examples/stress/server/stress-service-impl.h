#ifndef STRESS_SERVICE_IMPL_H
#define STRESS_SERVICE_IMPL_H

namespace google { namespace protobuf {
    class Service;
}}

namespace vtrc { namespace common {
    struct connection_iface;
}}

namespace stress {
    google::protobuf::Service *create_service(
                                            vtrc::common::connection_iface *c );
}

#endif // STRESS_SERVICE_IMPL_H
