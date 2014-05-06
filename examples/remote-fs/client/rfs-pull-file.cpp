
#include <iostream>
#include <fstream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

#include "vtrc-memory.h"

#include "utils.h"

namespace rfs_examples {

    using namespace vtrc;

    void pull_file( client::vtrc_client_sptr client,
                    vtrc::shared_ptr<interfaces::remote_fs> &impl,
                    const std::string &remote_path, size_t block_size )
    {
        size_t remote_size = -1;
        try {
            remote_size = impl->file_size( remote_path );
            std::cout << "Remote file size is: " << remote_size << "\n";
        } catch( const vtrc::common::exception &ex ) {
            std::cout << "Remote file size is unknown: "
                      << remote_path
                      << " '" << ex.what( ) << "; " << ex.additional( ) << "'"
                      << "\n";
        }

        std::string name = leaf_of( remote_path );
        vtrc::shared_ptr<interfaces::remote_file> rem_f
                ( interfaces::create_remote_file( client, remote_path, "rb" ) );

        std::cout << "Open remote file success.\n"
                  << "Starting...\n"
                  << "Block size = " << block_size
                  << std::endl;

        std::ofstream f;
        f.open(name.c_str( ), std::ofstream::out );

        std::string block;
        size_t total = 0;

        while( rem_f->read( block, block_size ) ) {
            total += block.size( );
            std::cout << "Pull " << total << " bytes\r";
            f.write( block.c_str( ), block.size( ) );
        }
        std::cout << "\nDownload complete\n";
    }

}
