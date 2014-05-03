#ifndef REMOTE_FS_IFACE_H
#define REMOTE_FS_IFACE_H

#include <string>
#include "vtrc-memory.h"

namespace vtrc { namespace client {
    class vtrc_client;
}}

namespace interfaces {

    struct remote_fs {

        virtual ~remote_fs( ) { }
        virtual void cd( const std::string &new_path ) const = 0;

        /// service
        virtual unsigned get_handle( ) const = 0;
    };

    remote_fs *create_remote_fs(
                vtrc::shared_ptr<vtrc::client::vtrc_client> client,
                std::string const &path );

}

#endif // REMOTE_FS_IFACE_H
