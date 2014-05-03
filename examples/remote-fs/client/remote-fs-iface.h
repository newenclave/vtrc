#ifndef REMOTE_FS_IFACE_H
#define REMOTE_FS_IFACE_H

#include <string>
#include "vtrc-memory.h"

namespace vtrc { namespace client {
    class vtrc_client;
}}

namespace interfaces {

    struct fs_info {
        bool is_exist_;
        bool is_directory_;
        bool is_empty_;
        bool is_regular_;
    };

    struct remote_fs {

        virtual ~remote_fs( ) { }
        virtual void cd( const std::string &new_path ) const = 0;
        virtual std::string pwd( ) const = 0;

        virtual bool exists( std::string const &path ) const = 0;
        virtual bool exists( ) const = 0;

        virtual fs_info info( std::string const &path ) const = 0;
        virtual fs_info info( ) const = 0;

        /// service
        virtual unsigned get_handle( ) const = 0;
    };

    remote_fs *create_remote_fs(
                vtrc::shared_ptr<vtrc::client::vtrc_client> client,
                std::string const &path );

}

#endif // REMOTE_FS_IFACE_H