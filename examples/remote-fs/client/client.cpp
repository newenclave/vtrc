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
        ("pwd,w", "Show current remote path")
        ("info,i", po::value<std::string>( ), "Show info about remote path")
        ("list,l", "list remote directory")
        ("tree,t", "show remote directory as tree")
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

std::string leaf_of( const std::string &path )
{
    std::string::const_reverse_iterator b(path.rbegin( ));
    std::string::const_reverse_iterator e(path.rend( ));
    for( ; b!=e ;++b ) {
        if( *b == '/' || *b == '\\' ) {
            return std::string( b.base( ), path.end( ) );
        }
    }
    return path;
}

void list_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl )
{
    vtrc::shared_ptr<interfaces::remote_fs_iterator> i(impl->begin_iterator( ));
    std::string lstring( 2, ' ' );
    for( ; !i->end( ); i->next( )) {
        bool is_dir( i->info( ).is_directory_ );
        std::cout << lstring
                  << ( i->info( ).is_empty_ ? " " : "+" );
        std::cout << ( is_dir ? "[" : " " )
                  << leaf_of(i->info( ).path_)
                  << ( is_dir ? "]" : " " )
                  << "\n";
    }
}

void tree_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl,
               const std::string &path,
               int level = 0 )
{
    vtrc::shared_ptr<interfaces::remote_fs_iterator> i
                                                (impl->begin_iterator( path ));
    std::string lstring( level * 2, ' ' );
    for( ; !i->end( ); i->next( )) {
        bool is_dir( i->info( ).is_directory_ );
        std::cout << lstring
                  << ( i->info( ).is_empty_ ? " " : "+" );
        if( is_dir ) {
            std::cout << "[" << leaf_of(i->info( ).path_) << "]\n";
            try {
                tree_dir( impl, i->info( ).path_, level + 1 );
            } catch( ... ) {
                std::cout << lstring << "  <iteration failed>\n";
            }
        } else if( i->info( ).is_symlink_ ) {
            std::cout << "<!" << leaf_of(i->info( ).path_) << ">\n";
        } else {
            std::cout << " " << leaf_of(i->info( ).path_) << "\n";
        }

    }
}

void tree_dir( vtrc::shared_ptr<interfaces::remote_fs> &impl )
{
    tree_dir( impl, "", 0 );
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
    std::cout << "Creating client ... " ;
    vtrc::condition_variable ready_cond;
    client->get_on_ready( ).connect( vtrc::bind( on_client_ready,
                            vtrc::ref( ready_cond ) ) );

    std::cout << "success\n";

    std::string path;

    connect_to( client, params["server"].as<std::string>( ) );

    std::cout << "Connected to "
              << params["server"].as<std::string>( ) << "\n";

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
        std::cout << "success.\nInfo:\n";
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
        list_dir( impl );
    }

    if( params.count( "tree" ) ) {
        std::cout << "Tree dir:\n";
        tree_dir( impl );
    }

    return 0;
}
