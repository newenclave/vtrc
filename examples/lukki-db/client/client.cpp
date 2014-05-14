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

#include "lukki-db-iface.h"

namespace po = boost::program_options;
using namespace vtrc;

void get_options( po::options_description& desc )
{
    desc.add_options( )
        ("help,?",   "help message")
        ("server,s", po::value<std::string>( ),
                     "server name; <tcp address>:<port> or <pipe/file name>")
        ( "set,S",   po::value<std::string>( ), "set value; "
                                            "use --value options for values")
        ( "del,D",   po::value<std::string>( ), "delete value")

        ( "upd,U",   po::value<std::string>( ), "update value; "
                                            "use --value options for values")
        ( "get,G",   po::value<std::string>( ), "get value")

        ("value,V",  po::value<std::vector< std::string> >( ), "values for "
                                                      " set or upd commands" )

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

void on_client_ready( vtrc::condition_variable &cond )
{
    std::cout << "Connection is ready...\n";
    cond.notify_all( );
}

int start( const po::variables_map &params )
{
    if( params.count( "server" ) == 0 ) {
        throw std::runtime_error("Server is not defined;\n"
                                 "Use --help for details");
    }

    /// will use only one thread for io operations.
    /// because we don't have callbacks or events from server-side
    common::pool_pair pp( 1 );

    std::cout << "Creating client ... " ;

    client::vtrc_client_sptr client = client::vtrc_client::create( pp );

    /// connect slot to 'on_ready'
    vtrc::condition_variable ready_cond;
    client->get_on_ready( ).connect( vtrc::bind( on_client_ready,
                            vtrc::ref( ready_cond ) ) );

    std::cout << "Ok\n";

    connect_to( client, params["server"].as<std::string>( ) );

    std::cout << "Connected to "
              << params["server"].as<std::string>( ) << "\n";

    vtrc::mutex                    ready_mutex;
    vtrc::unique_lock<vtrc::mutex> ready_lock(ready_mutex);
    ready_cond.wait( ready_lock,
                     vtrc::bind( &client::vtrc_client::ready, client ) );

    vtrc::shared_ptr<interfaces::luki_db> impl
                                        (interfaces::create_lukki_db(client));

    std::vector<std::string> values;
    if( params.count( "value" ) ) {
        values = params["value"].as< std::vector<std::string> >( );
    }

    if( params.count( "set" ) ) {
        std::string name(params["set"].as<std::string>( ));
        std::cout << "Set '" << name << "' to " << values.size( )
                  << " values...";
        impl->set( name, values );
        std::cout << "Ok\n";
    }

    if( params.count( "del" ) ) {
        std::string name(params["del"].as<std::string>( ));
        std::cout << "Delete '" << name << "'...";
        impl->del( name );
        std::cout << "Ok\n";
    }

    pp.stop_all( );
    pp.join_all( );

    return 0;
}
