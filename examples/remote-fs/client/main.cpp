
#include <iostream>
#include "boost/program_options.hpp"
#include "vtrc/common/exception.h"
#include "google/protobuf/descriptor.h"

namespace po = boost::program_options;

/// implementation is in client.cpp
int start( const po::variables_map &params );
void get_options( po::options_description& desc );


po::variables_map create_cmd_params(int argc, char *argv[],
                                    po::options_description const desc)
{
    po::variables_map vm;
    po::parsed_options parsed (
        po::command_line_parser(argc, argv)
            .options(desc)
            //.allow_unregistered( )
            .run( ));
    po::store(parsed, vm);
    po::notify(vm);
    return vm;
}

void show_help( po::options_description const &desc )
{
    std::cout << "Usage: remote_fs_client <options>\n"
              << "Options: \n" << desc;
}



int main( int argc, char *argv[] )
{
    po::options_description description("Allowed options");

    get_options( description );
    po::variables_map vm;

    try {
        vm = create_cmd_params( argc, argv, description );
    } catch ( const std::exception &ex ) {
        std::cerr << "Options error: " << ex.what( )
                  << "\n"
                  << description;
        return 1;
    }

    if( vm.count( "help" ) ) {
        show_help( description );
        return 0;
    }
    int err = 0;
    try {
        start( vm );
    } catch( const vtrc::common::exception &ex ) {
        std::cerr << "Client start failed: "
                  << ex.what( );
        std::string add(ex.additional( ));
        if( !add.empty( ) ) {
            std::cerr << " '" << add << "'";
        }
        std::cerr << "\n";
        err = 3;
    } catch( const std::exception &ex ) {
        std::cerr << "Client start failed: " << ex.what( ) << "\n";
        err = 3;
    }

    google::protobuf::ShutdownProtobufLibrary( );
    return err;
}

