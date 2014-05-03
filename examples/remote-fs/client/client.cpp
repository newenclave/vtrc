#include <vector>
#include <string>

#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"


namespace po = boost::program_options;
using namespace vtrc;

void get_options( po::options_description& desc )
{
    desc.add_options( )
        ("help,?",                                 "help message")
        ("server,s",    po::value<std::string>( ),
                        "endpoint name; <tcp address>:<port> or <pipe name>")

        ;
}

void connect( client::vtrc_client_sptr client, std::string const &server )
{
    std::vector<std::string> params;
    boost::split( params, server, boost::is_any_of(":") );

    if( params.size( ) > 3 ) {
        std::ostringstream oss;
        oss << "Invalid endpoint name: " << server;
        throw std::runtime_error( oss.str( ) );
    }

    if( params.size( ) == 1 ) { // local name
        client->connect( params[0] );
    } else if( params.size( ) == 2 ) {
        client->connect( params[0], params[1]);
    }

}

int start( const po::variables_map &params )
{

    if( params.count( "server" ) == 0 ) {
        throw std::runtime_error("Server is not defined;\n"
                                 "Use --help for details");
    }

    common::pool_pair pp(1, 1);
    client::vtrc_client_sptr client = client::vtrc_client::create( pp );


    return 0;
}
