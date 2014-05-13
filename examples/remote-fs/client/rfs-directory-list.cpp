

#include <iostream>
#include <fstream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

#include "vtrc-memory.h"

#include "utils.h"

namespace rfs_examples {

    using namespace vtrc;

    void list_dir( interfaces::remote_fs &impl)
    {
        typedef vtrc::shared_ptr<interfaces::remote_fs_iterator> iterator;

        size_t dirs(0);
        size_t files(0);
        std::string lstring( 2, ' ' );

        for( iterator i(impl.begin_iterator( )); !i->end( ); i->next( ) ) {
            bool is_dir( i->info( ).is_directory_ );

            dirs  += is_dir ? 1 : 0;
            files += is_dir ? 0 : 1;

            std::cout << lstring
                      << ( i->info( ).is_empty_ ? " " : "+" );
            std::cout << ( is_dir ? "[" : " " )
                      << leaf_of(i->info( ).path_)
                      << ( is_dir ? "]" : " " )
                      << "\n";
        }

        std::cout << "==================================================\n"
                  << "Total dirs:  " << dirs  << "\n"
                  << "Total files: " << files << "\n"
                  ;
    }

}
