
#include <iostream>
#include <fstream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

#include "vtrc-memory.h"

#include "utils.h"

#include "boost/filesystem.hpp"

#include "rfs-calls.h"

namespace rfs_examples {

    using namespace vtrc;
    namespace gpb = google::protobuf;
    namespace fs = boost::filesystem;

    void push_tree_impl( client::vtrc_client_sptr &client,
                         interfaces::remote_fs    &impl,
                         const fs::path           &remote_root,
                         const fs::path           &root,
                         const fs::path           &path )
    {
        fs::directory_iterator begin( root / path );

        impl.mkdir( path.string( ) );

        const fs::directory_iterator end;
        for( ;begin != end; ++begin ) {

            if( fs::is_regular_file(*begin) ) {


                fs::path remote_path( remote_root /
                                      path        /
                                      begin->path( ).leaf( ) );

                std::cout << "Push file: "
                             << "\"" << begin->path( ).string( ) << "\""
                             << " -> "
                             << "\"" << remote_path.string( ) << "\""
                             << "\n";

                push_file( client,
                           begin->path( ).string( ), remote_path.string( ),
                           44000 );
            } else if( fs::is_directory( *begin ) ) {

                std::cout << "dir: " << begin->path( ) << "\n";
                push_tree_impl( client, impl,
                                remote_root, root,
                                path / begin->path( ).leaf( ) );

            }
        }
    }

    void push_tree( client::vtrc_client_sptr &client,
                    interfaces::remote_fs    &impl,
                    const std::string &local_path )
    {
        fs::path path( local_path );
        fs::path remote_root( impl.pwd( ) );
        push_tree_impl( client, impl,
                        remote_root,
                        path.parent_path( ), path.leaf( ) );
    }
}
