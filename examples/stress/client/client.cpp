#include <vector>
#include <string>
#include <iostream>

#include "boost/program_options.hpp"
#include "boost/lexical_cast.hpp"

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-lowlevel-protocol-iface.h"

#include "vtrc-condition-variable.h"
#include "vtrc-mutex.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

#include "protocol/stress.pb.h"

#include "vtrc-chrono.h"
#include "vtrc-common/vtrc-delayed-call.h"

#include "stress-iface.h"
#include "ping-impl.h"
#include "events-impl.h"

#include "vtrc-thread.h"

#include "boost/thread.hpp" /// for thread_group

#if defined( _WIN32 )
#define sleep_ Sleep /// milliseconds
#define MILLISECONDS( x ) x
#else
#define sleep_ usleep /// microseconds
#define MILLISECONDS( x ) ((x) * 1000)
#endif

namespace po = boost::program_options;
namespace gpb = google::protobuf;

using namespace vtrc;

struct work_time {

    typedef vtrc::chrono::high_resolution_clock::time_point time_point;

    time_point start_;
    time_point total_;
    work_time( )
        :start_(vtrc::chrono::high_resolution_clock::now( ))
        ,total_(start_)
    { }

    std::string diff( const time_point &now, const time_point &last )
    {
        namespace cr = vtrc::chrono;
        std::ostringstream oss;
        oss << cr::duration_cast<cr::microseconds>(now - last).count( )
            << " microseconds";

        return oss.str( );
    }

    void print_point( const std::string &name )
    {
        time_point now(vtrc::chrono::high_resolution_clock::now( ));
        time_point::duration stop( now - start_);
        std::cout << "[" << name << "]"<< " call time: '" << stop.count( )
                  << "' total: '" << diff(now, total_) << "'\n";
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

        ("ping,p", po::value<unsigned>( ),  "make ping [arg] time")

        ("flood,f", po::value<unsigned>( ), "make ping [arg] time; no pause")

        ("threads,T", po::value<unsigned>( ), "additional threads for test"
                                              "; ping, flood, events, callback")

        ("gen-callbacks,c", po::value<unsigned>( ),
            "ask server for generate [arg] callbacks")

        ("gen-events,e", po::value<unsigned>( ),
            "ask server for generate [arg] events")

        ("recursive,R", po::value<unsigned>( ),
            "make recursion calls")

        ("payload,l", po::value<unsigned>( ),
            "payload in bytes for commands such as ping; default = 64")

        ("shutdown,S",  "shutdown remote server")

        ("timer",  po::value< std::vector<unsigned> >( ),
                    "register timer event" )
        ("sleep",  po::value< unsigned >( ), "sleep before exit" )

        ("key,k", po::value<std::string>( ),
                "key for access to server; password")

        ("close,Q",         "close this client" )

        ("dumb,D", "use dump protocol as lowlevel "
                   "(there are no handshakes or keys here)")

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

void start_ping_flood( vtrc::shared_ptr<stress::interface> impl,
                       bool flood, unsigned times, unsigned payload) try
{
    stress::ping( *impl, flood, times, payload );
} catch ( const std::exception &ex ) {
    std::cerr << "ping error: " << ex.what( ) << "\n";
}

void start_generate_events( vtrc::shared_ptr<stress::interface> impl,
                            unsigned count,
                            bool insert, bool wait,
                            unsigned payload) try
{
    impl->generate_events( count, insert, wait, payload );
} catch(  const std::exception &ex ) {
    std::cout << "generate_events error: " << ex.what( ) << "\n";
}

void start_recursive( vtrc::shared_ptr<stress::interface> impl,
                      unsigned count,
                      unsigned payload ) try
{
    impl->recursive_call( count, payload );

} catch(  const std::exception &ex ) {
    std::cout << "generate_events error: " << ex.what( ) << "\n";
}

void register_timers( vtrc::shared_ptr<stress::interface> impl,
                      const std::vector<unsigned> &timers)
{
    typedef std::vector<unsigned>::const_iterator iter;
    for( iter b(timers.begin( )), e(timers.end( )); b!=e; ++b ) {
        impl->register_timer( *b );
    }
}

int start( const po::variables_map &params )
{
    int err = 0;
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

    common::pool_pair pp( io_size, rpc_size - 1 );

    std::cout << "Creating client ... " ;

    client::vtrc_client_sptr client = client::vtrc_client::create( pp );

    if( params.count( "key" ) ) {
        std::string key = params["key"].as<std::string>( );
        client->set_session_key( key );
    }

    if( params.count( "dumb" ) ) {
        client->assign_lowlevel_protocol_factory(
                    &common::lowlevel::dummy::create );
    }

    client->on_connect_connect   ( on_connect );
    client->on_ready_connect     ( on_ready );
    client->on_disconnect_connect( on_disconnect );

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

    unsigned threads = params.count( "threads" )
            ? params["threads"].as<unsigned>( )
            : 0;

    boost::thread_group tg;

    if( params.count( "timer" ) ) {

        std::vector<unsigned> t(params["timer"].as< std::vector<unsigned> >( ));

        std::cout << "Register timers ... ";
        register_timers( impl, t );
        std::cout << "Ok\n";
    }

    std::vector<unsigned> timers;

    try {
    if( params.count( "shutdown" ) ) {

        std::cout << "Shutdown remote server...";
        impl->shutdown( );
        std::cout << "Ok\n";

    } if( params.count( "close" ) ) {
        std::cout << "Request server for close this client...";
        impl->close_me( );
        std::cout << "Ok\n";
    } else if( params.count( "ping" ) ) {

        unsigned times = params["ping"].as<unsigned>( );

        for( unsigned i=0; i<threads; ++i ) {
            tg.create_thread( vtrc::bind( start_ping_flood, impl,
                                          false, times, payload ) );
        }

        stress::ping( *impl, false, times, payload );

    } else if( params.count( "flood" ) ) {

        unsigned times = params["flood"].as<unsigned>( );

        for( unsigned i=0; i<threads; ++i ) {
            tg.create_thread( vtrc::bind( start_ping_flood,
                                          impl, true, times, payload ) );
        }

        stress::ping( *impl, true, times, payload );

    } else if( params.count( "gen-events" ) ) {

        vtrc::thread t;

        std::cout << "Ask server for events; "
                  << "payload: " << payload
                  << "\n";
        std::cout << "Current thread is: "
                  << this_thread::get_id( ) << "\n";

        vtrc::shared_ptr<gpb::Service> events(stress::create_events(
                                                            client, payload ));
        client->assign_rpc_handler( events );

        unsigned event_count = params["gen-events"].as<unsigned>( );

        for( unsigned i=0; i<threads; ++i ) {
            tg.create_thread( vtrc::bind( start_generate_events,
                                          impl, event_count,
                                          true, false, payload ));
        }

        start_generate_events(impl, event_count, true, false, payload);

        std::cout << "Ok\n";

    } else if ( params.count( "gen-callbacks" ) ) {

        std::cout << "Ask server for callbacks; "
                  << "payload: " << payload
                  << "\n";
        std::cout << "Current thread is: "
                  << this_thread::get_id( ) << "\n";

        vtrc::shared_ptr<gpb::Service> events(stress::create_events( client,
                                                                     payload ));
        client->assign_rpc_handler( events );

        unsigned event_count = params["gen-callbacks"].as<unsigned>( );

        for( unsigned i=0; i<threads; ++i ) {
            tg.create_thread( vtrc::bind( start_generate_events,
                                          impl, event_count,
                                          true, true, payload ) );
        }

        start_generate_events(impl, event_count, true, true, payload);

        std::cout << "Ok\n";

    } else if ( params.count( "recursive" ) ) {

        std::cout << "Ask server for recursive call\n";

        vtrc::shared_ptr<gpb::Service> events(stress::create_events( client,
                                                                     payload ));
        client->assign_rpc_handler( events );

        unsigned call_count = params["recursive"].as<unsigned>( );

        for( unsigned i=0; i<threads; ++i ) {
            tg.create_thread( vtrc::bind(start_recursive,
                                         impl, call_count, payload) );
        }

        start_recursive( impl, call_count, payload );

        std::cout << "Ok\n";

    }

    } catch( const vtrc::common::exception &ex ) {
        std::cerr << "Client process failed (v): "
                  << ex.what( );
        std::string add(ex.additional( ));
        if( !add.empty( ) ) {
            std::cerr << " '" << add << "'";
        }
        std::cerr << "\n";
        err = 3;
    } catch( const std::exception &ex ) {
        std::cerr << "Client process failed: " << ex.what( ) << "\n";
        err = 3;
    }

    if( params.count( "sleep" ) ) {
        unsigned value = params["sleep"].as<unsigned>( );
        sleep_( MILLISECONDS(value) * 1000 );
    }

    tg.join_all( );
    //pp.get_rpc_pool( ).attach( );
    pp.stop_all( );
    pp.join_all( );

    std::cout << "Stopped\n";

    google::protobuf::ShutdownProtobufLibrary( );
    return err;
}
