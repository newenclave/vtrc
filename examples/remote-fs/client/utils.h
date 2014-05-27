
#include <string>

namespace rfs_examples {

    std::string leaf_of( const std::string &path );

#ifdef _WIN32
    std::wstring make_ws_string_from_utf8( const std::string &src );
    std::wstring make_utf8_string( const std::wstring &src );
#endif

}

