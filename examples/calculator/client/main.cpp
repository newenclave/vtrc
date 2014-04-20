#include <iostream>

#include "vtrc-client-base/vtrc-client.h"
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
    work_time( )
        :start_(vtrc::chrono::high_resolution_clock::now( ))
    {}
    ~work_time( )
    {
        time_point::duration stop(
                    vtrc::chrono::high_resolution_clock::now( ) - start_);
        std::cout << "Call time: " << stop << "\n";
    }
};

class variable_pool: public vtrc_example::variable_pool {

    std::map<std::string, double> variables_;
    unsigned server_calback_count_;

    void set_variable(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::number* request,
                 ::vtrc_example::number* response,
                 ::google::protobuf::Closure* done)
    {
        ++server_calback_count_;

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
        ++server_calback_count_;
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
        :server_calback_count_(0)
    {}

    unsigned calls_count( ) const
    {
        return server_calback_count_;
    }

    void set( std::string const &name, double value )
    {
        /// don't need lock this calls;
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
    /// first: simple sum, mul, div, pow

    std::string splitter( 80, '=' );
    splitter += "\n";

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

    /**
     * ======================== THE TEST THAT FAILED ===================
    **/
    test_name = "failed";
    std::cout << splitter << test_name << ":\n";
    try {
        std::cout << "\tDivision by zero\n";

        double res = calc->div( "first", 0); /// CALL

        std::cout << "\tfirst/0 = " << res << "\n"; /// we don't see this out
        vars->set( test_name, res );

    } catch( const vtrc::common::exception& ex ) { /// but see this
        std::cout << "call '"<< test_name << "' failed: " << ex.what( ) << "; "
                  << ex.additional( ) << "\n";
    }

    /**
     * ======================== THE TEST THAT FAILED 2 =================
    **/
    test_name = "failed2";
    std::cout << splitter << test_name << ":\n";
    try {
        std::cout << "\tRequesting invalid variabe 'invalid'"
                  << " and make 'invalid' * 1\n";

        double res = calc->mul( "invalid", 1 ); /// CALL

        std::cout << "\t'invalid' * 1 = "
                  << res << "\n"; /// we don't see this out
        vars->set( test_name, res );

    } catch( const vtrc::common::exception& ex ) { /// but see this
        std::cout << "call '"<< test_name << "' failed: " << ex.what( ) << "; "
                  << ex.additional( ) << "\n";
    }

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

    std::cout << splitter;
    std::cout << "Total RPC was made: " << calc->calls_count( ) << "\n";
    std::cout << splitter;

}

void on_client_ready( vtrc::condition_variable &cond )
{
    cond.notify_all( );
}

int main( int argc, char **argv ) try
{

    if( argc < 2 ) {
        usage( );
        return 1;
    }

    /// one thread for transport io, and one for calls
    pool_pair pp(1, 1);

    /// create client
    vtrc::shared_ptr<vtrc_client> client(vtrc_client::create(pp));

    /// connect slot to 'on_ready'
    vtrc::condition_variable ready_cond;
    client->get_on_ready( ).connect( vtrc::bind( on_client_ready,
                            vtrc::ref( ready_cond ) ) );

    /// connecting client to server
    if( argc < 3 ) {
        std::cout << "connecting to '" << argv[1] << "'...";
        client->connect( argv[1] );
        std::cout << "success\n";
    } else {
        std::cout << "connecting to '"
                  << argv[1] << ":" << argv[2] << "'...";
        client->connect( argv[1], argv[2] );
        std::cout << "success\n";
    }

    /// set rpc handler for local variable
    vtrc::shared_ptr<variable_pool> vars(vtrc::make_shared<variable_pool>());
    client->assign_weak_rpc_handler( vtrc::weak_ptr<variable_pool>(vars) );


    /// wait for client ready; There must be a better way. But anyway ... :)))
    vtrc::mutex              ready_mutex;
    vtrc::unique_lock<vtrc::mutex> ready_lock(ready_mutex);
    ready_cond.wait( ready_lock, vtrc::bind( &vtrc_client::ready, client ) );

    /// add some variables
    vars->set( "pi", 3.1415926 );
    vars->set( "e",  2.7182818 );

    /// we call 'tests' in another thread, for example
    //vtrc::thread(tests, client, vars).join( ); /// and wait it

    /// or we can call it in main( )
    tests( client, vars );

    std::cout << "Server callbacks count: " << vars->calls_count( ) << "\n";
    std::cout << "\n\nFINISH\n\n";

    return 0;

} catch( const std::exception &ex ) {
    std::cout << "General client error: " << ex.what( ) << "\n";
    return 2;
}
