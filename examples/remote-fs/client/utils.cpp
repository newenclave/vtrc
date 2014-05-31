
#include <sstream>

#include "boost/format.hpp"
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

    std::string percents_string( double value, double maximum )
    {
        if( value > maximum ) value = maximum;
        double percents = value / (maximum / 100);
        size_t val = static_cast<size_t>(percents) >> 1;

        std::string complete = std::string( val ? (val - 1) : 0, '=' );
        std::string incomplete= std::string( val ? 50 - val : 50, '.' );
        std::string marker= std::string( val ? (val == 50 ? "=" : "]") : "" );
        std::ostringstream oss;
        oss << ( boost::format( "%|6.02f|%%" ) % percents )
            << " ["
            << complete << marker
            << incomplete << "]";
        return oss.str( );
    }
}
