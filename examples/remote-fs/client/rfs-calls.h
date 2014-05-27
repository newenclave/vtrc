#ifndef RFS_CALLS_H
#define RFS_CALLS_H

#include <string>

#include "rfs-iface.h"
#include "vtrc-client-base/vtrc-client.h"

namespace rfs_examples {

    /// rfs-directory-list.cpp
    void list_dir( interfaces::remote_fs &impl);

    /// rfs-directory-tree.cpp
    void tree_dir( interfaces::remote_fs &impl );

    /// rfs-push-file.cpp
    void push_file( vtrc::client::vtrc_client_sptr &client,
                const std::string &local_path, size_t block_size );

    void push_file( vtrc::client::vtrc_client_sptr &client,
                    const std::string &local_path,
                    const std::string &remote_path,
                    size_t block_size );

    /// rfs-push-tree.cpp
    void push_tree( vtrc::client::vtrc_client_sptr &client,
                    interfaces::remote_fs          &impl,
                    const std::string              &local_path );

    /// rfs-pull-file.cpp
    void pull_file( vtrc::client::vtrc_client_sptr &client,
                    interfaces::remote_fs          &impl,
                    const std::string &remote_path, size_t block_size );
    void pull_file( vtrc::client::vtrc_client_sptr &client,
                    interfaces::remote_fs          &impl,
                    const std::string &remote_path,
                    const std::string &local_path,
                    size_t block_size );

    /// rfs-push-tree.cpp
    void pull_tree(vtrc::client::vtrc_client_sptr &client,
                    interfaces::remote_fs          &impl,
                    const std::string              &remote_path );

    void pull_tree(vtrc::client::vtrc_client_sptr &client,
                    interfaces::remote_fs          &impl,
                    const std::string              &remote_path,
                    const std::string              &local_path);

    /// rfs-mkdir-del.cpp
    void make_dir( interfaces::remote_fs &impl, const std::string &path);
    void make_dir( interfaces::remote_fs &impl );
    void del( interfaces::remote_fs &impl, const std::string &path);
    void del( interfaces::remote_fs &impl );
}


#endif // RFSCALLS_H
