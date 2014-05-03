#include "protocol/remotefs.pb.h"
#include "vtrc-common/vtrc-connection-iface.h"

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

    google::protobuf::Service *create_fs_instance(
                            vtrc::common::connection_iface *client )
    {
        return new remote_fs_impl(client);
    }

    google::protobuf::Service *create_file_instance(
                            vtrc::common::connection_iface *client )
    {
        return NULL;
    }

}
