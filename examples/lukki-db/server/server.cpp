#include <string>
#include <vector>
#include <iostream>

#include "vtrc/server/application.h"

#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"

#include "vtrc/common/pool-pair.h"
#include "vtrc/common/rpc-service-wrapper.h"

#include "vtrc/server/listener-tcp.h"
#include "vtrc/server/listener-local.h"

#include "lukki-db-application.h"

namespace po = boost::program_options;
using namespace vtrc;


void get_options( po::options_description& desc )
{
    desc.add_options( )
        ("help,?",   "help message")
        ("server,s", po::value<std::vector< std::string> >( ),
                     "endpoint name; <tcp address>:<port> or <pipe/file name>")

        ("io-pool-size,i",  po::value<unsigned>( ), "threads for io operations"
                                                    "; default = 1")
        ;
}

server::listener_sptr create_from_string( const std::string &name,
                                          server::application &app)
{
    /// result endpoint
    server::listener_sptr result;

    std::vector<std::string> params;

    size_t delim_pos = name.find_last_of( ':' );
    if( delim_pos == std::string::npos ) {

        /// local: <localname>
        params.push_back( name );

    } else {

        /// tcp: <addres>:<port>
        params.push_back( std::string( name.begin( ),
                                       name.begin( ) + delim_pos ) );

        params.push_back( std::string( name.begin( ) + delim_pos + 1,
                                       name.end( ) ) );
    }

    if( params.size( ) == 1 ) {         /// local endpoint
#ifndef _WIN32
        ::unlink( params[0].c_str( ) ); /// unlink old file socket
#endif
        result = server::listeners::local::create( app, params[0] );

    } else if( params.size( ) == 2 ) {  /// TCP

        result = server::listeners::tcp::create( app,
                        params[0],
                        boost::lexical_cast<unsigned short>(params[1]));

    }
    return result;
}

int start( const po::variables_map &params )
{
    if( params.count( "server" ) == 0 ) {
        throw std::runtime_error("Server endpoint is not defined;\n"
                                 "Use --help for details");
    }

    /// as far as we use only ONE thread for DB access
    /// we don't need rpc-pool anymore
    unsigned io_size = params.count( "io-pool-size" )
            ? params["io-pool-size"].as<unsigned>( )
            : 1;

    if( io_size < 1 ) {
        throw std::runtime_error( "io-pool-size must be at least 1" );
    }

    typedef std::vector<std::string> string_vector;
    string_vector servers = params["server"].as<string_vector>( );

    common::pool_pair pp( io_size - 1, 1 );
    lukki_db::application app( pp );

    for( string_vector::const_iterator b(servers.begin( )), e(servers.end( ));
                    b != e; ++b)
    {
        std::cout << "Starting listener at '" <<  *b << "'...";
        app.attach_start_listener( create_from_string( *b, app ) );
        std::cout << "Ok\n";
    }

    pp.get_io_pool( ).attach( );
    pp.join_all( );

    return 0;
}
