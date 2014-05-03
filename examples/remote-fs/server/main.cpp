#include <iostream>
#include "boost/program_options.hpp"
#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-pool-pair.h"

using namespace vtrc;
namespace po = boost::program_options;

class fs_application: public server::application {
public:
    fs_application( common::pool_pair &pp )
        :server::application(pp)
    { }
};

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

    description.add_options( )
        ("help,?",                                 "help message")
        ("server,s",    po::value<std::string>( ), "endpoint name; "
                                                   "tcp address or pipe name")
        ("port,p",      po::value<std::string>( ), "port for tcp server")
        ("pass,k",      po::value<std::string>( ), "password for remote clients")
        ;

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

    if( vm.count( "server" ) == 0 ) {
        std::cerr << "Server endpoint is not defined\n";
        show_help( description );
        return 2;
    }

    common::pool_pair pp( 1, 1 );
    fs_application app( pp );

    pp.stop_all( );
    pp.join_all( );
    return 0;
}
