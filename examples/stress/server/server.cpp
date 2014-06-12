#include <string>
#include <vector>

#include "boost/program_options.hpp"
#include "stress-application.h"

namespace po = boost::program_options;
using namespace vtrc;

void get_options( po::options_description& desc )
{
    desc.add_options( )

        ("help,?",   "help message")

        ("server,s", po::value<std::vector< std::string> >( ),
            "endpoint name; <tcp address>:<port> or <pipe/file name>")

        ("io-pool-size,i",  po::value<unsigned>( ),
            "threads for io operations; default = 1")

        ("rpc-pool-size,r", po::value<unsigned>( ),
            "threads for rpc calls; default = 1")

        ("tcp-nodelay,t",
            "set TCP_NODELAY flag for tcp sockets")

        ("accept-retry,A", po::value<unsigned>( ),
            "retry time out if accept is failed; milliseconds")

        ("only-pool,o",
            "use io pool for io operations and rpc calls")

        ("message-size,M", po::value<unsigned>( ),
            "maximum message size for protocol")

        ("max-calls,C", po::value<unsigned>( ),
            "maximum calls per user")

        ("read-size,B", po::value<unsigned>( ),
            "buffer size for receiving data")

        ("stack-size,S", po::value<unsigned>( ),
            "maximum recursion depth for both side")

        ;
}

int start( const po::variables_map &params )
{
    if( params.count( "server" ) == 0 ) {
        throw std::runtime_error("Server endpoint is not defined;\n"
                                 "Use --help for details");
    }

    bool use_only_pool = !!params.count( "only-pool" );

    unsigned io_size = params.count( "io-pool-size" )
            ? params["io-pool-size"].as<unsigned>( )
            : ( use_only_pool ? 0 : 1 );

    unsigned rpc_size = params.count( "rpc-pool-size" )
            ? params["rpc-pool-size"].as<unsigned>( )
            : 1;

    if( (rpc_size < 1) && !use_only_pool) {
        throw std::runtime_error( "rpc-pool-size must be at least 1" );
    }

    vtrc::shared_ptr<stress::application> app;

    if( use_only_pool ) {
        app = vtrc::make_shared<stress::application>( io_size );
    } else {
        app = vtrc::make_shared<stress::application>( io_size, rpc_size - 1 );
    }

    app->run( params );

    return 0;
}
