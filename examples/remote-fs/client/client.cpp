#include <vector>
#include <string>

#include <fstream>


#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-exception.h"

#include "rfs-iface.h"

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
        ("pwd,w", "Show current remote path")
        ("info,i", po::value<std::string>( ), "Show info about remote path")
        ("list,l", "list remote directory")
        ("tree,t", "show remote directory as tree")

        ("pull,g", po::value<std::string>( ), "Download remote file")
        ("push,u", po::value<std::string>( ), "Upload local file")

        ("block-size,b", po::value<unsigned>( ), "Block size for pull and push"
                                                 "; 1-640000")

        ("mkdir,m", po::value<std::string>( ), "Create remote directory")
        ("del,d",   po::value<std::string>( ), "Remove remote directory"
                                               " if it is empty")
        ;
}

void connect_to( client::vtrc_client_sptr client, std::string const &name )
{
    std::vector<std::string> params;

    std::string::const_reverse_iterator b(name.rbegin( ));
    std::string::const_reverse_iterator e(name.rend( ));
    for( ; b!=e ;++b ) {
        if( *b == ':' ) {
            std::string::const_reverse_iterator bb(b);
            ++bb;
            params.push_back( std::string( name.begin( ), bb.base( )) );
            params.push_back( std::string( b.base( ), name.end( )));

            break;
        }
    }

    if( params.empty( ) ) {
        params.push_back( name );
    }

    if( params.size( ) == 1 ) {        /// local name
        client->connect( params[0] );
    } else if( params.size( ) == 2 ) { /// tcp
        client->connect( params[0], params[1]);
    }

}

void on_client_ready( vtrc::condition_variable &cond )
{
    std::cout << "Connection is ready...\n";
    cond.notify_all( );
}

namespace rfs_examples {

    /// rfs-directory-list.cpp
    void list_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl);

    /// rfs-directory-tree.cpp
    void tree_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl );

    /// rfs-push-file.cpp
    void push_file( client::vtrc_client_sptr client,
                const std::string &local_path, size_t block_size );

    /// rfs-pull-file.cpp
    void pull_file( client::vtrc_client_sptr client,
                    vtrc::shared_ptr<interfaces::remote_fs> &impl,
                    const std::string &remote_path, size_t block_size );

    /// rfs-mkdir-del.cpp
    void make_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl,
                   const std::string &path);
    void make_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl );
    void del( vtrc::shared_ptr<interfaces::remote_fs> &impl,
                   const std::string &path);
    void del( vtrc::shared_ptr<interfaces::remote_fs> &impl );
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
        rfs_examples::list_dir( impl );
    }

    if( params.count( "tree" ) ) {
        std::cout << "Tree dir:\n";
        rfs_examples::tree_dir( impl );
    }

    size_t bs = params.count( "block-size" )
            ? params["block-size"].as<unsigned>( )
            : 4096;

    if( params.count( "pull" ) ) {
        std::string path = params["pull"].as<std::string>( );
        std::cout << "pull file '" << path << "'\n";
        rfs_examples::pull_file( client, impl, path, bs );
    }

    if( params.count( "push" ) ) {
        std::string path = params["push"].as<std::string>( );
        std::cout << "push file '" << path << "'\n";
        rfs_examples::push_file( client, path, bs );
    }

    if( params.count( "mkdir" ) ) {
        std::string path = params["mkdir"].as<std::string>( );
        if( path.empty( ) ) {
            rfs_examples::make_dir( impl );
        } else {
            rfs_examples::make_dir( impl, path );
        }
    }

    if( params.count( "del" ) ) {
        std::string path = params["del"].as<std::string>( );
        if( path.empty( ) ) {
            rfs_examples::del( impl );
        } else {
            rfs_examples::del( impl, path );
        }
    }

    impl.reset( ); // close fs instance
    pp.stop_all( );
    pp.join_all( );

    return 0;
}
