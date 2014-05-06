

#include <iostream>
#include <fstream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

#include "vtrc-memory.h"

#include "utils.h"

namespace rfs_examples {

    using namespace vtrc;

    void list_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl )
    {
        vtrc::shared_ptr<interfaces::remote_fs_iterator> i
                                                    (impl->begin_iterator( ));
        std::string lstring( 2, ' ' );
        for( ; !i->end( ); i->next( )) {
            bool is_dir( i->info( ).is_directory_ );
            std::cout << lstring
                      << ( i->info( ).is_empty_ ? " " : "+" );
            std::cout << ( is_dir ? "[" : " " )
                      << leaf_of(i->info( ).path_)
                      << ( is_dir ? "]" : " " )
                      << "\n";
        }
    }

}
