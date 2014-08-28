#ifndef REMOTE_FS_IFACE_H
#define REMOTE_FS_IFACE_H

#include <string>
#include <vector>
#include "google/protobuf/stubs/common.h"
#include "vtrc-memory.h"

namespace vtrc { namespace client {
    class vtrc_client;
}}

namespace interfaces {

    struct fs_info {
        std::string path_;
        bool        is_exist_;
        bool        is_directory_;
        bool        is_empty_;
        bool        is_regular_;
        bool        is_symlink_;
    };

    struct remote_fs_iterator {

        virtual ~remote_fs_iterator( ) { }

        virtual const fs_info &info( )       = 0;
        virtual void next( )                 = 0;
        virtual bool end( )            const = 0;
        virtual vtrc::shared_ptr<remote_fs_iterator> clone( ) const = 0;
    };

    struct remote_file {
        virtual ~remote_file( ) { }

        virtual const std::string &path( )       const = 0;
        virtual void close( )                    const = 0;

        virtual google::protobuf::uint64 tell( ) const = 0;

        virtual google::protobuf::uint64 seek( google::protobuf::uint64 pos,
                            unsigned whence )     const = 0;
        virtual google::protobuf::uint64 seek_begin(
                            google::protobuf::uint64 pos ) const = 0;
        virtual google::protobuf::uint64 seek_end(
                            google::protobuf::uint64 pos )   const = 0;

        virtual size_t read( std::string &data, size_t max_len ) const = 0;

        virtual size_t write( std::string &data )               const = 0;
        virtual size_t write( const char *data, size_t lenght ) const = 0;

    };

    struct remote_fs {

        virtual ~remote_fs( ) { }
        virtual void cd( const std::string &new_path ) const = 0;
        virtual std::string pwd( ) const = 0;

        virtual bool exists( std::string const &path ) const = 0;
        virtual bool exists( ) const = 0;

        virtual fs_info info( std::string const &path ) const = 0;
        virtual fs_info info( ) const = 0;

        virtual google::protobuf::uint64 file_size(
                                            std::string const &path ) const = 0;
        virtual google::protobuf::uint64 file_size( ) const = 0;

        virtual void mkdir( std::string const &path ) const = 0;
        virtual void mkdir( ) const = 0;

        virtual void del( std::string const &path ) const = 0;
        virtual void del( ) const = 0;

        /// iterator start
        virtual vtrc::shared_ptr<remote_fs_iterator>
                               begin_iterator(const std::string &path) const =0;
        virtual vtrc::shared_ptr<remote_fs_iterator> begin_iterator( ) const =0;

        /// service
        virtual unsigned get_handle( ) const = 0;
    };

    remote_fs *create_remote_fs(
                vtrc::shared_ptr<vtrc::client::vtrc_client> client,
                std::string const &path );

    remote_file *create_remote_file(
            vtrc::shared_ptr<vtrc::client::vtrc_client> client,
            std::string const &path, std::string const &mode );
}

#endif // REMOTE_FS_IFACE_H
