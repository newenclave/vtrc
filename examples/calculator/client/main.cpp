#include <iostream>

#include "boost/lexical_cast.hpp"

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-exception.h"

#include "protocol/calculator.pb.h"
#include "calculator-iface.h"

#include "vtrc-condition-variable.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"
#include "vtrc-thread.h"
#include "vtrc-chrono.h"

using namespace vtrc::client;
using namespace vtrc::common;

void usage(  )
{
    std::cout
          << "Usage: \n\tcalculator_client <server ip> <server port>\n"
          << "or:\n\tcalculator_client <localname>\n"
          << "examples:\n"
          << "\tfor tcp: calculator_client 127.0.0.1 55555\n"
          << "\tfor unix: calculator_client ~/calculator.sock\n"
          << "\tfor win pipe: calculator_client \\\\.\\pipe\\calculator_pipe\n";
}

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


/// variable_pool for server.
/// server can request variable by name
///
class variable_pool: public vtrc_example::variable_pool {

    std::map<std::string, double> variables_;
    unsigned server_callback_count_;

    void set_variable(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::number* request,
                 ::vtrc_example::number* response,
                 ::google::protobuf::Closure* done)
    {
        ++server_callback_count_;

        closure_holder done_holder(done);
        std::string n(request->name( ));

        std::cout << "Server sets variable: '" << n << "'"
                  << " = " <<  request->value( )
                  << " thread id: " << vtrc::this_thread::get_id( ) << "\n";

        variables_[n] = request->value( );
    }

    void get_variable(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::number* request,
                 ::vtrc_example::number* response,
                 ::google::protobuf::Closure* done)
    {
        ++server_callback_count_;
        closure_holder done_holder(done);
        std::string n(request->name( ));

        std::cout << "Server wants variable: '" << n << "'"
                  << "; thread id: " << vtrc::this_thread::get_id( ) << "\n";

        std::map<std::string, double>::const_iterator f(variables_.find(n));
        if( f == variables_.end( ) ) {
            std::cout << "Client sends exception to server...\n";
            std::ostringstream oss;
            oss << "Variable is not found: '" << n << "'";
            throw std::runtime_error( oss.str( ) );
        }
        response->set_value( f->second );
    }

public:

    variable_pool( )
        :server_callback_count_(0)
    { }

    unsigned calls_count( ) const
    {
        return server_callback_count_;
    }

    void set( std::string const &name, double value )
    {
        /// don't need lock this calls;
        std::cout << "Set '" << name << "' = " << value << "\n";
        variables_[name] = value;
    }

};

void tests( vtrc::shared_ptr<vtrc_client> client,
            vtrc::shared_ptr<variable_pool> vars )
{

    work_time wt;

    /// create calculator client interface
    vtrc::unique_ptr<interfaces::calculator>
                            calc(interfaces::create_calculator( client ));

    std::string test_name;

    std::string splitter( std::string(80, '=').append( "\n" ) );

    std::cout << "My thread id: " << vtrc::this_thread::get_id( ) << "\n";

    /**
     * ======================== FIRST ==================================
    **/
    test_name = "first";
    std::cout << splitter << test_name << ":\n";
    try {

        double res = calc->mul( "pi", "e" ); /// CALL

        std::cout << "\tpi * e = " << res << "\n";
        vars->set( test_name, res ); /// now we can use old result

    } catch( const vtrc::common::exception& ex ) {
        std::cout << "call '"<< test_name << "' failed: " << ex.what( ) << "; "
                  << ex.additional( ) << "\n";
    }
    wt.print_point( test_name );

    /**
     * ======================== SECOND =================================
    **/
    test_name = "second";
    std::cout << splitter << test_name << ":\n";
    try {

        double res = calc->sum( 17, calc->mul( 3, 5 )); /// CALL

        std::cout << "\t17 + 3 * 5 = " << res << "\n";
        vars->set( test_name, res );

    } catch( const vtrc::common::exception& ex ) {
        std::cout << "call '"<< test_name << "' failed: " << ex.what( ) << "; "
                  << ex.additional( ) << "\n";
    }
    wt.print_point( test_name );

    /**
     * ======================== THRID ==================================
    **/
    test_name = "third";
    std::cout << splitter << test_name << ":\n";
    try {
        std::cout << "\tnow we take first and seconds results "
                  << "and make pow(firts, second)\n";

        double res = calc->pow( "first", "second" ); /// CALL

        std::cout << "\tpow(first, second) = " << res << "\n";
        vars->set( test_name, res );

    } catch( const vtrc::common::exception& ex ) {
        std::cout << "call '"<< test_name << "' failed: " << ex.what( ) << "; "
                  << ex.additional( ) << "\n";
    }
    wt.print_point( test_name );

    /**
     * ======================== THE TEST THAT FAILED 1 =================
    **/
    test_name = "Division by zero";
    std::cout << splitter << test_name << ":\n";
    try {

        double res = calc->div( "first", 0); /// FAIL CALL

        std::cout << "\tfirst/0 = " << res << "\n"; /// we don't see this out
        vars->set( test_name, res );

    } catch( const vtrc::common::exception& ex ) { /// but see this
        std::cout << "call '"<< test_name << "' failed: " << ex.what( ) << "; "
                  << ex.additional( ) << "\n";
    }
    wt.print_point( test_name );

    /**
     * ======================== THE TEST THAT FAILED 2 =================
    **/
    test_name = "Invalid variable";
    std::cout << splitter << test_name << ":\n";
    try {
        std::cout << "\tRequesting invalid variabe 'invalid'"
                  << " and make 'invalid' * 1\n";

        double res = calc->mul( "invalid", 1 ); /// FAIL CALL

        std::cout << res << "\n";

    } catch( const vtrc::common::exception& ex ) { /// but see this
        std::cout << "call '"<< test_name << "' failed: " << ex.what( ) << "; "
                  << ex.additional( ) << "\n";
    }
    wt.print_point( test_name );

    /**
     * ======================== THE TEST THAT FAILED 3 =================
    **/
    test_name = "unimplemented call";
    std::cout << splitter << test_name << ":\n";
    try {
        std::cout << "\tMaking call that unimplemented on server-side\n";

        calc->not_implemented( ); /// CALL

    } catch( const vtrc::common::exception& ex ) { /// but see this
        std::cout << "call '"<< test_name << "' failed: " << ex.what( ) << "; "
                  << ex.additional( ) << "\n";
    }
    wt.print_point( test_name );

    std::cout << splitter;
    std::cout << "Total RPC was made: " << calc->calls_count( ) << "\n";
    std::cout << splitter;

}

int main( int argc, char **argv ) try
{

    if( argc < 2 ) {
        usage( );
        return 1;
    }

    /// one thread for transport io, and one for calls
    pool_pair pp(2, 3);

    /// create client
    vtrc::shared_ptr<vtrc_client> client(vtrc_client::create(pp));

    /// connecting client to server
    if( argc < 3 ) {
        std::cout << "connecting to local '" << argv[1] << "'...";
        client->connect( argv[1] );
        std::cout << "Ok\n";
    } else {
        std::cout << "connecting to tcp '"
                  << argv[1] << ":" << argv[2] << "'...";
        client->connect( argv[1], boost::lexical_cast<unsigned short>(argv[2]));
        std::cout << "Ok\n";
    }

    /// set rpc handler for local variable
    vtrc::shared_ptr<variable_pool> vars(vtrc::make_shared<variable_pool>());
    client->assign_weak_rpc_handler( vtrc::weak_ptr<variable_pool>(vars) );

    /// add some variables
    vars->set( "pi", 3.1415926 );
    vars->set( "e",  2.7182818 );

    /// we call 'tests' in another thread, for example
    //vtrc::thread(tests, client, vars).join( ); /// and wait it

    /// or we can call it in main( )
    tests( client, vars );

    std::cout << "Server callbacks count: " << vars->calls_count( ) << "\n";
    std::cout << "\n\nFINISH\n\n";

    pp.stop_all( );
    pp.join_all( );

    google::protobuf::ShutdownProtobufLibrary( );
    return 0;

} catch( const std::exception &ex ) {
    google::protobuf::ShutdownProtobufLibrary( );
    std::cout << "General client error: " << ex.what( ) << "\n";
    return 2;
}
