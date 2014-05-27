#include <vector>
#include <string>

#include <fstream>

#include "boost/program_options.hpp"
#include "boost/lexical_cast.hpp"

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

#include "vtrc-condition-variable.h"
#include "vtrc-mutex.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

#include "rfs-calls.h"

namespace po = boost::program_options;
using namespace vtrc;

void get_options( po::options_description& desc )
{
    desc.add_options( )
        ("help,?",   "help message")
        ("server,s", po::value<std::string>( ),
                     "server name; <tcp address>:<port> or <pipe/file name>")
        ("path,p", po::value<std::string>( ), "Init remote path for client")
        ("pwd,w", "Show current remote path")
        ("info,i", po::value<std::string>( ), "Show info about remote path")
        ("list,l", "list remote directory")
        ("tree,t", "show remote directory as tree")

        ("pull,g", po::value<std::string>( ), "Download remote file")
        ("push,u", po::value<std::string>( ), "Upload local file")

        ("pull-tree,G", po::value<std::string>( ),
            "Download remote fs tree; "
            "Use --output for target local directory. default is './'")
        ("push-tree,U", po::value<std::string>( ), "Upload local fs tree")

        ("output,o", po::value<std::string>( ),
            "Target path; for pull-tree it means local directory")

        ("block-size,b", po::value<unsigned>( ), "Block size for pull and push"
                                                 "; 1-65000")

        ("mkdir,m", po::value<std::string>( ), "Create remote directory")
        ("del,d",   po::value<std::string>( ), "Remove remote directory"
                                               " if it is empty")
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

    std::string path;

    connect_to( client, params["server"].as<std::string>( ) );

    std::cout << "Connected to "
              << params["server"].as<std::string>( ) << "\n";

    vtrc::mutex                    ready_mutex;
    vtrc::unique_lock<vtrc::mutex> ready_lock(ready_mutex);
    ready_cond.wait( ready_lock,
                     vtrc::bind( &client::vtrc_client::ready, client ) );

    if( params.count( "path" ) ) {
        path = params["path"].as<std::string>( );
    }

    std::cout << "Path is: '" << path << "'\nCreating interface...";

    vtrc::shared_ptr<interfaces::remote_fs> impl
            (interfaces::create_remote_fs(client, path));

    std::cout << "Success; id is '" << impl->get_handle( ) << "'\n";

    std::cout << "Path " << ( impl->exists(  ) ? "exists" : "not exists" )
              << "\n";

    if( params.count( "pwd" ) ) {
        std::cout << "PWD: " << impl->pwd( ) << "\n";
    }

    std::string output;

    if( params.count( "output" ) ) {
        output = params["output"].as<std::string>( );
    }


    if( params.count( "info" ) ) {
        std::string pi(params["info"].as<std::string>( ));
        std::cout << "Trying get info about '" << pi << "'...";
        interfaces::fs_info inf = impl->info( pi );
        std::cout << "Ok.\nInfo:\n";
        std::cout << "\texists:\t\t"
                  << (inf.is_exist_ ? "true" : "false") << "\n";
        std::cout << "\tdirectory:\t"
                  << (inf.is_directory_ ? "true" : "false") << "\n";
        std::cout << "\tempty:\t\t"
                  << (inf.is_empty_ ? "true" : "false") << "\n";
        std::cout << "\tregular:\t"
                  << (inf.is_regular_ ? "true" : "false") << "\n";
    }

    if( params.count( "list" ) ) {
        std::cout << "List dir:\n";
        rfs_examples::list_dir( *impl );
    }

    if( params.count( "tree" ) ) {
        std::cout << "Tree dir:\n";
        rfs_examples::tree_dir( *impl );
    }

    size_t bs = params.count( "block-size" )
            ? params["block-size"].as<unsigned>( )
            : 4096;

    if( params.count( "pull" ) ) {
        std::string path = params["pull"].as<std::string>( );
        std::cout << "pull file '" << path << "'\n";
        rfs_examples::pull_file( client, *impl, path, bs );
    }

    if( params.count( "push" ) ) {
        std::string path = params["push"].as<std::string>( );
        std::cout << "push file '" << path << "'\n";
        rfs_examples::push_file( client, path, bs );
    }

    if( params.count( "push-tree" ) ) {
        std::string path = params["push-tree"].as<std::string>( );
        std::cout << "push tree '" << path << "'\n";
        rfs_examples::push_tree( client, *impl, path );
    }

    if( params.count( "pull-tree" ) ) {
        std::string path = params["pull-tree"].as<std::string>( );

        if( !output.empty( ) ) {
            std::cout << "pull tree '" << path << "'"
                      << " to " << output << "\n";
            rfs_examples::pull_tree( client, *impl, path, output );
        } else {
            std::cout << "pull tree '" << path << "'\n";
            rfs_examples::pull_tree( client, *impl, path);
        }

    }

    if( params.count( "mkdir" ) ) {
        std::string path = params["mkdir"].as<std::string>( );
        if( path.empty( ) ) {
            rfs_examples::make_dir( *impl );
        } else {
            rfs_examples::make_dir( *impl, path );
        }
    }

    if( params.count( "del" ) ) {
        std::string path = params["del"].as<std::string>( );
        if( path.empty( ) ) {
            rfs_examples::del( *impl );
        } else {
            rfs_examples::del( *impl, path );
        }
    }

    impl.reset( ); // close fs instance
    pp.stop_all( );
    pp.join_all( );

    return 0;
}
