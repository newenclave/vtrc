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

#include "vtrc-chrono.h"

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
        ( "set,S",   po::value<std::string>( ), "set value; "
                                            "use --value options for values")
        ( "del,D",   po::value<std::string>( ), "delete value")

        ( "upd,U",   po::value<std::string>( ), "update value if it exist; "
                                            "use --value options for values")
        ( "get,G",   po::value<std::string>( ), "get value")

        ( "value,V", po::value<std::vector< std::string> >( ),  "values for "
                                                      " set or upd commands;"
                           " -V \"value1\", -V \"value2\", -V \"value3\" ...")

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
                  << " values...\n";

        work_time wt;
        impl->set( name, values );
        wt.print_point( "set" );

        std::cout << "Ok\n";
    }

    if( params.count( "upd" ) ) {
        std::string name(params["upd"].as<std::string>( ));
        std::cout << "Update '" << name << "' to " << values.size( )
                  << " values...\n";
        work_time wt;
        impl->upd( name, values );
        wt.print_point( "update" );

        std::cout << "Ok\n";
    }

    if( params.count( "get" ) ) {
        std::string name(params["get"].as<std::string>( ));
        std::cout << "Get '" << name << "'...\n";

        work_time wt;
        std::vector<std::string> vals(impl->get( name ));
        wt.print_point( "get" );

        std::cout << "Ok\n";
        std::cout << "vals.size( ) = " << vals.size( ) << "\n==========\n";
        std::copy( vals.begin( ), vals.end( ),
                   std::ostream_iterator<std::string>( std::cout, "\n" ) );
        std::cout << "==========\n";

    }

    if( params.count( "del" ) ) {
        std::string name(params["del"].as<std::string>( ));
        std::cout << "Delete '" << name << "'...\n";

        work_time wt;
        impl->del( name );
        std::cout << "Ok\n";
        wt.print_point( "delete" );
    }


    pp.stop_all( );
    pp.join_all( );

    return 0;
}
