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
        ("help,?",   "help message")
        ("server,s", po::value<std::string>( ),
                     "server name; <tcp address>:<port> or <pipe/file name>")

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

    /// will use only one thread for io operations.
    /// because we don't have callbacks or events from server-side
    common::pool_pair pp( 1, 0 );

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

//    vtrc::shared_ptr<interfaces::lukki_db> impl
//                                        (interfaces::create_lukki_db(client));

    pp.join_all( );
    std::cout << "Stopped\n";

    google::protobuf::ShutdownProtobufLibrary( );
    return 0;
}
