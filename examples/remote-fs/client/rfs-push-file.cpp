
#include <iostream>
#include <string>
#include <fstream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

#include "vtrc-memory.h"

#include "utils.h"

namespace rfs_examples {

    using namespace vtrc;

    void push_file( vtrc::client::vtrc_client_sptr &client,
                const std::string &local_path,
                const std::string &remote_path,
                size_t block_size )
    {
        vtrc::shared_ptr<interfaces::remote_file> rem_f
                ( interfaces::create_remote_file( client, remote_path, "wb" ) );

        std::cout << "Open remote file success.\n"
                  << "Starting...\n"
                  << "Block size = " << block_size
                  << std::endl;

        std::ifstream f;
        f.open(local_path.c_str( ), std::ifstream::in );

        std::string block(block_size, 0);
        size_t total = 0;

        while( size_t r = f.readsome( &block[0], block_size ) ) {
            size_t shift = 0;
            while ( r ) {
                size_t w = rem_f->write( block.c_str( ) + shift, r );
                total += w;
                shift += w;
                r -= w;
                std::cout << "Push " << total << " bytes\r";
            }
        }
        std::cout << "\nUpload complete\n";
    }

    void push_file( client::vtrc_client_sptr &client,
                    const std::string &local_path, size_t block_size )
    {
        std::string name = leaf_of( local_path );
        push_file( client, local_path, name, block_size );
    }

}
