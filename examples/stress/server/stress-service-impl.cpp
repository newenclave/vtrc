#include "stress-service-impl.h"
#include "protocol/stress.pb.h"
#include "vtrc-common/vtrc-connection-iface.h"

namespace gpb = google::protobuf;
using namespace vtrc;

namespace  {

    class stress_service_impl: public vtrc_example::stress_service {
        common::connection_iface *c_;

    public:

        stress_service_impl( common::connection_iface *c )
            :c_(c)
        { }

    private:

    };

}

namespace stress {

    gpb::Service *create_service( common::connection_iface *c )
    {
        return new stress_service_impl( c );
    }

}
