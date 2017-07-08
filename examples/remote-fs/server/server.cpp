#include <string>
#include <vector>
#include <iostream>

#include "vtrc/server/application.h"

#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"

#include "vtrc/server/application.h"
#include "vtrc/common/pool-pair.h"
#include "remote-fs-impl.h"
#include "protocol/remotefs.pb.h"

#include "vtrc-rpc-lowlevel.pb.h"

#include "vtrc/common/rpc-service-wrapper.h"

#include "vtrc/server/listener-tcp.h"
#include "vtrc/server/listener-local.h"

#include "vtrc-bind.h"
#include "vtrc-ref.h"

namespace po = boost::program_options;
using namespace vtrc;

class fs_application: public server::application {
    typedef fs_application this_type;
public:
    fs_application( common::pool_pair &pp )
        :server::application(pp)
    { }

    ~fs_application( )
    {
        google::protobuf::ShutdownProtobufLibrary( );
    }

    void configure_session( common::connection_iface* connection,
                                    rpc::session_options &opts )
    {
        opts.set_read_buffer_size( 44000 );
    }

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

        /// empty service;
        return vtrc::shared_ptr<common::rpc_service_wrapper>( );
    }

    void on_new_connection( server::listener_sptr l,
                            const common::connection_iface *c )
    {
        std::cout << "New connection: "
                  << "\n\tep:     " << l->name( )
                  << "\n\tclient: " << c->name( )
                  << "\n"
                    ;
    }

    void on_stop_connection( server::listener_sptr l,
                             const common::connection_iface *c )
    {
        std::cout << "Close connection: "
                  << c->name( ) << "\n";
    }

    void attach_listener( server::listener_sptr listen )
    {
        listen->on_new_connection_connect(
                    vtrc::bind( &this_type::on_new_connection, this,
                                listen, vtrc::placeholders::_1 ));
        listen->on_stop_connection_connect(
                    vtrc::bind( &this_type::on_stop_connection, this,
                                listen, vtrc::placeholders::_1 ));
    }

};

void get_options( po::options_description& desc )
{
    desc.add_options( )
        ("help,?",   "help message")
        ("server,s", po::value<std::vector< std::string> >( ),
                     "endpoint name; <tcp address>:<port> or <pipe/file name>")

        ("io-pool-size,i",  po::value<unsigned>( ), "threads for io operations"
                                                    "; default = 1")
        ("rpc-pool-size,r", po::value<unsigned>( ), "threads for rpc calls"
                                                    "; default = 1")

        ;
}

server::listener_sptr create_from_string( const std::string &name,
                                          server::application &app)
{
    /// result endpoint
    server::listener_sptr result;

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

    if( params.size( ) == 1 ) {         /// local endpoint
#ifndef _WIN32
        ::unlink( params[0].c_str( ) ); /// unlink old file socket
#endif
        result = server::listeners::local::create( app, params[0] );

    } else if( params.size( ) == 2 ) {  /// TCP

        result = server::listeners::tcp::create( app,
                        params[0],
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

    typedef std::vector<std::string> string_vector;
    string_vector servers = params["server"].as<string_vector>( );

    common::pool_pair pp( io_size, rpc_size - 1 );
    fs_application app( pp );

    std::vector<server::listener_sptr> listeners;

    listeners.reserve(servers.size( ));

    for( string_vector::const_iterator b(servers.begin( )), e(servers.end( ));
                    b != e; ++b)
    {
        std::cout << "Starting listener at '" <<  *b << "'...";
        server::listener_sptr new_listener = create_from_string( *b, app );
        listeners.push_back(new_listener);
        app.attach_listener( new_listener );
        new_listener->start( );
        std::cout << "Ok\n";
    }

    pp.get_rpc_pool( ).attach( );
    pp.join_all( );

    return 0;
}
