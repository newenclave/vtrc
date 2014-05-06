

#include <iostream>
#include <fstream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

#include "vtrc-memory.h"

#include "utils.h"

namespace rfs_examples {

    using namespace vtrc;

namespace {
    void tree_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl,
                   const std::string &path, int level,
                   size_t &dirs, size_t &files )
    {
        vtrc::shared_ptr<interfaces::remote_fs_iterator> i
                                                (impl->begin_iterator( path ));
        std::string lstring( level * 2, ' ' );
        for( ; !i->end( ); i->next( )) {

            bool is_dir( i->info( ).is_directory_ );

            dirs  += is_dir ? 1 : 0;
            files += is_dir ? 0 : 1;

            std::cout << lstring
                      << ( i->info( ).is_empty_ ? " " : "+" );

            if( is_dir ) {
                std::cout << "[" << leaf_of(i->info( ).path_) << "]\n";
                try {
                    tree_dir( impl, i->info( ).path_, level + 1, dirs, files );
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
        size_t dirs(0);
        size_t files(0);
        tree_dir( impl, path, 0, dirs, files );
        std::cout << "==================================================\n"
                  << "Total dirs:  " << dirs  << "\n"
                  << "Total files: " << files << "\n"
                  ;
    }

    void tree_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl )
    {
        tree_dir( impl, "" );
    }

}

