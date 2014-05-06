

#include <iostream>
#include <fstream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-exception.h"

#include "remote-fs-iface.h"

#include "vtrc-memory.h"

#include "utils.h"

namespace rfs_examples {

    using namespace vtrc;

namespace {
    void tree_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl,
                   const std::string &path, int level )
    {
        vtrc::shared_ptr<interfaces::remote_fs_iterator> i
                                                (impl->begin_iterator( path ));
        std::string lstring( level * 2, ' ' );
        for( ; !i->end( ); i->next( )) {
            bool is_dir( i->info( ).is_directory_ );
            std::cout << lstring
                      << ( i->info( ).is_empty_ ? " " : "+" );
            if( is_dir ) {
                std::cout << "[" << leaf_of(i->info( ).path_) << "]\n";
                try {
                    tree_dir( impl, i->info( ).path_, level + 1 );
                } catch( ... ) {
                    std::cout << lstring << "  <iteration failed>\n";
                }
            } else if( i->info( ).is_symlink_ ) {
                std::cout << "<!" << leaf_of(i->info( ).path_) << ">\n";
            } else {
                std::cout << " " << leaf_of(i->info( ).path_) << "\n";
            }

        }
    }
}

    void tree_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl,
                   const std::string &path )
    {
        tree_dir( impl, path, 0 );
    }

    void tree_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl )
    {
        tree_dir( impl, "" );
    }

}

