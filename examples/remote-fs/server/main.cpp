#include <iostream>
#include "boost/program_options.hpp"

namespace po = boost::program_options;

/// implementation is in server.cpp
int start( const po::variables_map &params );
void get_options( po::options_description& desc );

po::variables_map create_cmd_params(int argc, char *argv[],
                                    po::options_description const desc)
{
    po::variables_map vm;
    po::parsed_options parsed (
        po::command_line_parser(argc, argv)
            .options(desc)
            .allow_unregistered( )
            .run());
    po::store(parsed, vm);
    po::notify(vm);
    return vm;
}

void show_help( po::options_description const &desc )
{
    std::cout << "Usage: remote_fs_server <options>\n"
              << "Options: \n" << desc;
}

int main( int argc, char *argv[] )
{
    po::options_description description("Allowed common options");

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

    try {
        start( vm );
    } catch( const std::exception &ex ) {
        std::cerr << "Server start failed: " << ex.what( ) << "\n";
        return 3;
    }

    return 0;
}
