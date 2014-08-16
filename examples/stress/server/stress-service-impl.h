#ifndef STRESS_SERVICE_IMPL_H
#define STRESS_SERVICE_IMPL_H

#include <string>

namespace google { namespace protobuf {
    class Service;
}}

namespace vtrc { namespace common {
    struct connection_iface;
}}


namespace stress {

    class application;

    google::protobuf::Service *create_service(
                               vtrc::common::connection_iface *c,
                               application &app);

    std::string service_name( );
}

#endif // STRESS_SERVICE_IMPL_H
