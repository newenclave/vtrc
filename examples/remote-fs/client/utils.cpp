
#include "utils.h"

namespace rfs_examples {
    std::string leaf_of( const std::string &path )
    {
        std::string::const_reverse_iterator b(path.rbegin( ));
        std::string::const_reverse_iterator e(path.rend( ));
        for( ; b!=e ;++b ) {
            if( *b == '/' || *b == '\\' ) {
                return std::string( b.base( ), path.end( ) );
            }
        }
        return path;
    }
}
