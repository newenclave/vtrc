
#include <iostream>
#include <fstream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

#include "vtrc-memory.h"

#include "utils.h"

namespace rfs_examples {

    using namespace vtrc;
    namespace gpb = google::protobuf;

    void pull_file( vtrc::client::vtrc_client_sptr &client,
                    const interfaces::remote_fs    &impl,
                    const std::string &remote_path,
                    const std::string &local_path,
                    size_t block_size )
    {
        gpb::uint64 remote_size = -1;
        try {
            remote_size = impl.file_size( remote_path );
            std::cout << "Remote file size is: " << remote_size << "\n";
        } catch( const vtrc::common::exception &ex ) {
            std::cout << "Remote file size is unknown: "
                      << remote_path
                      << " '" << ex.what( ) << "; " << ex.additional( ) << "'"
                      << "\n";
        }

        vtrc::shared_ptr<interfaces::remote_file> rem_f
                ( interfaces::create_remote_file( client, remote_path, "rb" ) );

        std::cout << "Open remote file success.\n"
                  << "Starting...\n"
                  << "Block size = " << block_size
                  << std::endl;

        std::ofstream f;
        f.open(local_path.c_str( ), std::ofstream::out );

        std::string block;
        size_t total = 0;

        while( rem_f->read( block, block_size ) ) {

            total += block.size( );

            double percents = (remote_size == gpb::uint64(-1))
                            ? 100.0
                            : 100.0 - (double(remote_size - total )
                                      / (double(remote_size) / 100));

            std::cout << "Pull " << total << " bytes "
                      << percents_string( percents, 100.0 ) << "\r"
                      << std::flush;

            f.write( block.c_str( ), block.size( ) );
        }
        std::cout << "\nDownload complete\n";
    }

    void pull_file( client::vtrc_client_sptr &client,
                    const interfaces::remote_fs &impl,
                    const std::string &remote_path, size_t block_size )
    {
        std::string name = leaf_of( remote_path );
        pull_file( client, impl, remote_path, name, block_size );
    }

}
