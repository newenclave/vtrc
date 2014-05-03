#ifndef REMOTE_FS_IMPL_H
#define REMOTE_FS_IMPL_H

#include "vtrc-common/vtrc-connection-iface.h"

namespace google { namespace protobuf {
    class Service;
} }


namespace remote_fs {

    google::protobuf::Service *create_fs_instance(
                            vtrc::common::connection_iface *client );

    google::protobuf::Service *create_file_instance(
                            vtrc::common::connection_iface *client );

}

#endif // REMOTEFSIMPL_H
