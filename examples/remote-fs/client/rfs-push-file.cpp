
#include <iostream>
#include <string>
#include <fstream>

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

#include "vtrc-memory.h"

#include "utils.h"

#include "boost/filesystem.hpp"

namespace rfs_examples {

    using namespace vtrc;
    namespace gpb = google::protobuf;

    void push_file( vtrc::client::vtrc_client_sptr &client,
                const std::string &local_path,
                const std::string &remote_path,
                size_t block_size )
    {
        vtrc::shared_ptr<interfaces::remote_file> rem_f
                ( interfaces::create_remote_file( client, remote_path, "wb" ) );

        gpb::uint64 file_size = gpb::uint64(-1);

        try {
            file_size = boost::filesystem::file_size( local_path );
            std::cout << "File size is: " << file_size << "\n";
        } catch( const std::exception &ex ) {
            std::cout << "File size is unknown: "
                      << local_path
                      << " '" << ex.what( ) << "\n";
        }

        std::cout << "Open remote file success.\n"
                  << "Starting...\n"
                  << "Block size = " << block_size
                  << std::endl;

        std::string block(block_size, 0);
        size_t total = 0;

        if( local_path == "-" ) {

            std::cout << "Input: \n";

            while( fgets( &block[0], block_size, stdin ) ) {

                size_t r = strlen(&block[0]);
                size_t shift = 0;

                while ( r ) {
                    size_t w = rem_f->write( block.c_str( ) + shift, r );
                    total += w;
                    shift += w;
                    r     -= w;
                }
                rem_f->flush( );
            }

        } else {

            std::ifstream f;
            f.open(local_path.c_str( ), std::ifstream::in );

            while( size_t r = f.readsome( &block[0], block_size ) ) {

                size_t shift = 0;

                while ( r ) {

                    size_t w = rem_f->write( block.c_str( ) + shift, r );
                    total += w;
                    shift += w;
                    r     -= w;

                    double percents = (file_size == gpb::uint64(-1))
                                    ? 100.0
                                    : 100.0 - (double(file_size - total )
                                              / (double(file_size) / 100));

                    std::cout << "Push " << total << " bytes "
                              << percents_string( percents, 100.0 ) << "\r"
                              << std::flush;
                }
            }
        }
        std::cout << "\nUpload complete\n";
    }

    void push_file( client::vtrc_client_sptr &client,
                    const std::string &local_path,
                    size_t block_size )
    {
        std::string name = leaf_of( local_path );
        push_file( client, local_path, name, block_size );
    }

}
