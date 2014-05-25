#include <vector>
#include <string>

#include "boost/program_options.hpp"
#include "boost/lexical_cast.hpp"

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-exception.h"

#include "vtrc-condition-variable.h"
#include "vtrc-mutex.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

#include "protocol/stress.pb.h"

#include "stress-iface.h"
#include "ping-impl.h"

#include "vtrc-chrono.h"
#include "vtrc-common/vtrc-delayed-call.h"

namespace po = boost::program_options;
using namespace vtrc;

struct work_time {

    typedef vtrc::chrono::high_resolution_clock::time_point time_point;

    time_point start_;
    time_point total_;
    work_time( )
        :start_(vtrc::chrono::high_resolution_clock::now( ))
        ,total_(start_)
    { }

    void print_point( const std::string &name )
    {
        time_point now(vtrc::chrono::high_resolution_clock::now( ));
        time_point::duration stop( now - start_);
        std::cout << "[" << name << "]"<< " call time: '" << stop
                  << "' total: '" << (now - total_) << "'\n";
        start_ = vtrc::chrono::high_resolution_clock::now( );
    }

    ~work_time( )
    { }
};


void get_options( po::options_description& desc )
{
    desc.add_options( )

        ("help,?", "help message")

        ("server,s", po::value<std::string>( ),
            "server name; <tcp address>:<port> or <pipe/file name>")

        ("io-pool-size,i",  po::value<unsigned>( ),
            "threads for io operations; default = 1")

        ("rpc-pool-size,r",  po::value<unsigned>( ),
            "threads for rpc calls; default = 1")

        ("tcp-nodelay,t",
            "set TCP_NODELAY flag for tcp sockets")

        ("ping,p", po::value<unsigned>( ), "make ping [arg] time")

        ("payload,l", po::value<unsigned>( ),
            "payload in bytes for commands such as ping; "
            "dafault = 64")

        ;
}

void connect_to( client::vtrc_client_sptr client, std::string const &name )
{
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

    if( params.size( ) == 1 ) {        /// local name
        client->connect( params[0] );
    } else if( params.size( ) == 2 ) { /// tcp
        client->connect( params[0],
                         boost::lexical_cast<unsigned short>(params[1]));
    }

}


namespace {
    void on_connect( )
    {
        std::cout << "connect...";
    }

    void on_ready( )
    {
        std::cout << "ready...";
    }

    void on_disconnect( )
    {
        std::cout << "disconnect...";
    }
}

int start( const po::variables_map &params )
{
    if( params.count( "server" ) == 0 ) {
        throw std::runtime_error("Server is not defined;\n"
                                 "Use --help for details");
    }

    unsigned io_size = params.count( "io-pool-size" )
            ? params["io-pool-size"].as<unsigned>( )
            : 1;

    if( io_size < 1 ) {
        throw std::runtime_error( "io-pool-size must be at least 1" );
    }

    unsigned rpc_size = params.count( "rpc-pool-size" )
            ? params["rpc-pool-size"].as<unsigned>( )
            : 1;

    if( rpc_size < 1 ) {
        throw std::runtime_error( "rpc-pool-size must be at least 1" );
    }

    common::pool_pair pp( io_size, rpc_size );

    std::cout << "Creating client ... " ;

    client::vtrc_client_sptr client = client::vtrc_client::create( pp );

    client->get_on_connect( )   .connect( on_connect );
    client->get_on_ready( )     .connect( on_ready );
    client->get_on_disconnect( ).connect( on_disconnect );

    std::cout << "Ok\n";

    std::cout << "Connecting to "
              << params["server"].as<std::string>( ) << "...";

    connect_to( client, params["server"].as<std::string>( ) );

    std::cout << "Ok\n";

    vtrc::shared_ptr<stress::interface> impl(
                stress::create_stress_client( client ));

    unsigned payload = params.count( "payload" )
            ? params["payload"].as<unsigned>( )
            : 64;

    if( params.count( "ping" ) ) {
        unsigned times = params["ping"].as<unsigned>( );
        stress::ping( *impl, times, payload, pp );
    }

    pp.join_all( );
    std::cout << "Stopped\n";

    google::protobuf::ShutdownProtobufLibrary( );
    return 0;
}
