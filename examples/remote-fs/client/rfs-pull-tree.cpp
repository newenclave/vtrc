
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

    namespace fs = boost::filesystem;

    void pull_tree( vtrc::client::vtrc_client_sptr &client,
                    const interfaces::remote_fs    &impl,
                    const std::string              &remote_path,
                    const std::string              &local_path)
    {
        typedef vtrc::shared_ptr<interfaces::remote_fs_iterator> iterator;

        iterator i = impl.begin_iterator(remote_path);

        std::cout << "Create dirs: " << local_path << "...";
        fs::create_directories( local_path );
        std::cout << "Ok\n";

        for( ; !i->end( ); i->next( ) ) {

            bool is_dir( i->info( ).is_directory_ );

            std::string name( leaf_of(i->info( ).path_) );
            fs::path next( local_path );
            next /= name;

            if( is_dir ) {
                try {
                    pull_tree( client, impl, i->info( ).path_, next.string( ) );
                } catch( ... ) {
                    std::cout << "<iteration failed>\n";
                }
            } else if( i->info( ).is_regular_ ) {
                std::cout << "Pull file: \"" <<  i->info( ).path_ << "\""
                          << " -> " << next
                          << "\n";
                pull_file( client, impl,
                           i->info( ).path_, next.string( ),
                           44000 );
            }
        }
    }

    void pull_tree(vtrc::client::vtrc_client_sptr &client,
                const interfaces::remote_fs       &impl,
                const std::string                 &remote_path )
    {
        fs::path local_path( remote_path );
        pull_tree( client, impl, remote_path, local_path.leaf( ).string( ) );
    }


}
