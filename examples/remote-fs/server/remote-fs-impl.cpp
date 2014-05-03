#include "protocol/remotefs.pb.h"
#include "vtrc-common/vtrc-connection-iface.h"
#include "google/protobuf/descriptor.h"

namespace {

    enum handle_type_index {
         handle_fs       = 0
        ,handle_iterator = 1
        ,handle_file     = 2

        ,handle_last
    };

    class remote_fs_impl: public vtrc_example::remote_fs {

        vtrc::common::connection_iface *connection_;

    public:
        remote_fs_impl(vtrc::common::connection_iface *connection)
            :connection_(connection)
        { }
    };
}

namespace remote_fs {
    google::protobuf::Service *create(vtrc::common::connection_iface *client )
    {
        return new remote_fs_impl( client );
    }
    const std::string name( )
    {
        return vtrc_example::remote_fs::descriptor( )->full_name( );
    }
}

namespace remote_file {

    google::protobuf::Service *create(vtrc::common::connection_iface *client )
    {
        return NULL;
    }

    const std::string name( )
    {
        return vtrc_example::remote_file::descriptor( )->full_name( );
    }
}
