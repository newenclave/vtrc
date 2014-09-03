#include <vector>
#include <string>
#include <iostream>

#include "boost/program_options.hpp"
#include "boost/lexical_cast.hpp"

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-exception.h"

#include "vtrc-condition-variable.h"
#include "vtrc-mutex.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

#include "lukki-db-iface.h"

#include "protocol/lukkidb.pb.h"

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
        ("help,?",   "help message")
        ("server,s", po::value<std::string>( ),
                     "server name; <tcp address>:<port> or <pipe/file name>")

        ( "stat,I",                             "get db status")
        ( "events,e",                           "subscribe and show events")

        ( "exist,E", po::value<std::string>( ), "checks if record exist")
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

class lukki_events_impl: public vtrc_example::lukki_events {

    void subscribed(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::empty* request,
                 ::vtrc_example::empty* response,
                 ::google::protobuf::Closure* done)
    {
        common::closure_holder ch(done);
        std::cout << "Subscribed\n";
    }

    void new_value(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::name_req* request,
                 ::vtrc_example::empty* response,
                 ::google::protobuf::Closure* done)
    {
        common::closure_holder ch(done);
        std::cout << "New record: '" << request->name( ) << "'\n";
    }

    void value_changed(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::name_req* request,
                 ::vtrc_example::empty* response,
                 ::google::protobuf::Closure* done)
    {
        common::closure_holder ch(done);
        std::cout << "Record '" << request->name( ) << "' was changed\n";
    }

    void value_removed(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::name_req* request,
                 ::vtrc_example::empty* response,
                 ::google::protobuf::Closure* done)
    {
        common::closure_holder ch(done);
        std::cout << "Record '" << request->name( ) << "' was removed\n";
    }
};

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
    /// for events we gonna attach 'main' thread
    common::pool_pair pp( 1, 0 );

    std::cout << "Creating client ... " ;

    client::vtrc_client_sptr client = client::vtrc_client::create( pp );

    client->on_connect_connect   ( on_connect );
    client->on_ready_connect     ( on_ready );
    client->on_disconnect_connect( on_disconnect );

    std::cout << "Ok\n";

    std::cout << "Connecting to "
              << params["server"].as<std::string>( ) << "...";

    connect_to( client, params["server"].as<std::string>( ) );

    std::cout << "Ok\n";

    vtrc::shared_ptr<interfaces::lukki_db> impl
                                        (interfaces::create_lukki_db(client));

    std::vector<std::string> values;
    bool events = (params.count( "events" ) != 0);

    if( events ) {
        std::cout << "Subscribing to events...";
        client->assign_rpc_handler( vtrc::make_shared<lukki_events_impl>( ) );
        impl->subscribe( );
        std::cout << "Ok\n";
    }

    if( params.count( "value" ) ) {
        values = params["value"].as< std::vector<std::string> >( );
    }

    if( params.count( "exist" ) ) {
        std::string name(params["exist"].as<std::string>( ));
        std::cout << "Check if '" << name << "' exists...\n";
        work_time wt;
        bool res = impl->exist( name );
        wt.print_point( "exist" );

        std::cout << "'" << name << "' "
                  << (res ? "exists" : "doesn't exist") << "\n";
    }

    if( params.count( "set" ) ) {

        std::string name(params["set"].as<std::string>( ));
        std::cout << "Set '" << name << "' to " << values.size( )
                  << " values...\n";

        work_time wt;
        impl->set( name, values );
        wt.print_point( "set" );

        std::cout << "Ok\n";

    } else if( params.count( "upd" ) ) {

        std::string name(params["upd"].as<std::string>( ));
        std::cout << "Update '" << name << "' to " << values.size( )
                  << " values...\n";
        work_time wt;
        impl->upd( name, values );
        wt.print_point( "update" );

        std::cout << "Ok\n";

    } else if( params.count( "get" ) ) {

        std::string name(params["get"].as<std::string>( ));
        std::cout << "Get '" << name << "'...\n";

        work_time wt;
        std::vector<std::string> vals(impl->get( name ));
        wt.print_point( "get" );

        std::cout << "Ok\n";

        std::cout << "got " << vals.size( )
                  << " value" << (vals.size( ) == 1 ? "" : "s")
                  << "\n==========\n";

        std::copy( vals.begin( ), vals.end( ),
                   std::ostream_iterator<std::string>( std::cout, "\n" ) );

        std::cout <<   "==========\n";

    } else if( params.count( "del" ) ) {

        std::string name(params["del"].as<std::string>( ));
        std::cout << "Delete '" << name << "'...\n";

        work_time wt;
        impl->del( name );
        wt.print_point( "delete" );

        std::cout << "Ok\n";
    }

    if( params.count( "stat" ) ) {

        std::cout << "Get db stat\n";
        work_time wt;
        vtrc_example::db_stat s(impl->stat( ));
        wt.print_point( "stat" );
        std::cout << "Ok\n";

        std::cout << "Stat:\n"
                  << "\tRecords: \t" << s.total_records( ) << "\n"
                  << "Requests:\n"
                  << "\t'set': \t\t" << s.set_requests( ) << "\n"
                  << "\t'upd': \t\t" << s.upd_requests( ) << "\n"
                  << "\t'get': \t\t" << s.get_requests( ) << "\n"
                  << "\t'del': \t\t" << s.del_requests( ) << "\n"
                  ;
    }

    if( !events ) {
        client->disconnect( );
        pp.stop_all( );
        std::cout << "Waiting for stop...";
    } else {
        std::cout << "Waiting for events...\n";
        /// ok we can have some events. Wait for them!
        pp.get_rpc_pool( ).attach( );
    }

    pp.join_all( );
    std::cout << "Stopped\n";

    google::protobuf::ShutdownProtobufLibrary( );
    return 0;
}
