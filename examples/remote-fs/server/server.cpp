#include <string>
#include <vector>

#include "vtrc-server/vtrc-application.h"
#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"

#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "remote-fs-impl.h"
#include "protocol/remotefs.pb.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"

#include "vtrc-server/vtrc-listener-tcp.h"
#include "vtrc-server/vtrc-listener-unix-local.h"
#include "vtrc-server/vtrc-listener-win-pipe.h"

namespace po = boost::program_options;
using namespace vtrc;

class fs_application: public server::application {
public:
    fs_application( common::pool_pair &pp )
        :server::application(pp)
    { }

    vtrc::shared_ptr<common::rpc_service_wrapper>
             get_service_by_name( common::connection_iface* connection,
                                  const std::string &service_name )
    {
        if( service_name == remote_fs::name( ) ) {
            return vtrc::make_shared<common::rpc_service_wrapper>
                    (remote_fs::create(connection));
        } else if(service_name == remote_file::name( )) {
            return vtrc::make_shared<common::rpc_service_wrapper>
                    (remote_file::create(connection));
        }
    }

};

void get_options( po::options_description& desc )
{
    desc.add_options( )
        ("help,?",                                 "help message")
        ("server,s",    po::value<std::vector< std::string> >( ),
                        "endpoint name; <tcp address>:<port> or <pipe name>")
        ;
}

server::listener_sptr create_from_string( const std::string &name,
                                          server::application &app)
{
    std::vector<std::string> params;
    boost::split( params, name, boost::is_any_of(":") );
    server::listener_sptr result;
    if( params.size( ) > 3 ) {
        std::ostringstream oss;
        oss << "Invalid endpoint name: " << name;
        throw std::runtime_error( oss.str( ) );
    }

    if( params.size( ) == 1 ) { // local name
#ifndef _WIN32
        ::unlink( params[0].c_str( ) );
        result = server::listeners::unix_local::create( app, params[0] );
#else
        result = server::listeners::win_pipe::create( app, params[0] );
#endif
    } else if( params.size( ) == 2 ) {
        result = server::listeners::tcp::create( app, params[0],
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

    typedef std::vector<std::string> string_vector;
    string_vector servers = params["server"].as< string_vector >( );

    common::pool_pair pp( 1, 1 );
    fs_application app( pp );

    std::vector<server::listener_sptr> listeners;

    listeners.reserve(servers.size( ));

    for( string_vector::const_iterator b(servers.begin( )), e(servers.end( ));
                    b != e; ++b)
    {
        listeners.push_back( create_from_string( *b, app ) );
        listeners.back( )->start( );
    }

    pp.stop_all( );
    pp.join_all( );

    return 0;
}