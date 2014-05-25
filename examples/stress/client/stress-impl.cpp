#include "stress-iface.h"

namespace stress {

namespace {

    struct impl: public interface {

        impl( vtrc::shared_ptr<vtrc::client::vtrc_client> &client )
        { }

        void ping( unsigned payload )
        {

        }

    };
}

    interface *create_stress_client(
            vtrc::shared_ptr<vtrc::client::vtrc_client> client )
    {
        return new impl( client );
    }

}

