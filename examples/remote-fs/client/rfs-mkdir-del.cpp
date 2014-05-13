
#include <iostream>
#include <fstream>

#include "rfs-iface.h"

namespace rfs_examples {

    void make_dir( interfaces::remote_fs &impl, const std::string &path)
    {
        std::cout << "Making remote directory '" << path << "'...";
        impl.mkdir( path );
        std::cout << "Ok\n";
    }

    void make_dir( interfaces::remote_fs &impl )
    {
        std::cout << "Making remote directory '" << impl.pwd( ) << "'...";
        impl.mkdir( );
        std::cout << "Ok\n";
    }

    void del( interfaces::remote_fs &impl, const std::string &path)
    {
        std::cout << "Removing remote fs object '" << path << "'...";
        impl.del( path );
        std::cout << "Ok\n";
    }

    void del( interfaces::remote_fs &impl )
    {
        std::cout << "Removing remote fs object '" << impl.pwd( ) << "'...";
        impl.del( );
        std::cout << "Ok\n";
    }

}
