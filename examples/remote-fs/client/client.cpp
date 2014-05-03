#include <vector>
#include <string>

#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "remote-fs-iface.h"

#include "vtrc-condition-variable.h"
#include "vtrc-mutex.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

namespace po = boost::program_options;
using namespace vtrc;

void get_options( po::options_description& desc )
{
    desc.add_options( )
        ("help,?",   "help message")
        ("server,s", po::value<std::string>( ),
                     "server name; <tcp address>:<port> or <pipe/file name>")
        ("path,p", po::value<std::string>( ), "Init remote path for client")
        ;
}

void connect_to( client::vtrc_client_sptr client, std::string const &server )
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

void on_client_ready( vtrc::condition_variable &cond )
{
    cond.notify_all( );
}

int start( const po::variables_map &params )
{

    if( params.count( "server" ) == 0 ) {
        throw std::runtime_error("Server is not defined;\n"
                                 "Use --help for details");
    }

    common::pool_pair pp(1, 1);
    client::vtrc_client_sptr client = client::vtrc_client::create( pp );

    /// connect slot to 'on_ready'
    vtrc::condition_variable ready_cond;
    client->get_on_ready( ).connect( vtrc::bind( on_client_ready,
                            vtrc::ref( ready_cond ) ) );

    std::string path;

    connect_to( client, params["server"].as<std::string>( ) );

    vtrc::mutex                    ready_mutex;
    vtrc::unique_lock<vtrc::mutex> ready_lock(ready_mutex);
    ready_cond.wait( ready_lock, vtrc::bind( &client::vtrc_client::ready,
                                              client ) );

    if( params.count( "path" ) ) {
        path = params["path"].as<std::string>( );
    }

    std::cout << "Path is: '" << path << "'\nCreating interface...";

    vtrc::shared_ptr<interfaces::remote_fs> impl
            (interfaces::create_remote_fs(client, path));

    std::cout << "Success; id is '" << impl->get_handle( ) << "\n";

    return 0;
}
