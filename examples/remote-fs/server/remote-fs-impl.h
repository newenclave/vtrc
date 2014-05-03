#ifndef REMOTE_FS_IMPL_H
#define REMOTE_FS_IMPL_H

#include "vtrc-common/vtrc-connection-iface.h"

namespace google { namespace protobuf {
    class Service;
}}

namespace remote_fs {
    google::protobuf::Service *create(vtrc::common::connection_iface *client );
    std::string name( );
}

namespace remote_file {
    google::protobuf::Service *create(vtrc::common::connection_iface *client );
    std::string name( );
}

#endif // REMOTEFSIMPL_H
