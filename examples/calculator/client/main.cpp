#include <iostream>

#include "vtrc-client-base/vtrc-client.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "protocol/calculator.pb.h"
#include "calculator-iface.h"

#include "vtrc-condition-variable.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

using namespace vtrc::client;
using namespace vtrc::common;

void usage(  )
{
    std::cout
          << "Usage: \n\tcalculator_client <server ip> <server port>\n"
          << "or:\n\tcalculator_client <localname>\n"
          << "examples:\n"
          << "\tfor tcp: calculator_client 127.0.0.1 55555\n"
          << "\tfor unix: calculator_client /tmp/calculator.sock\n"
          << "\tfor win pipe: calculator_client \\\\.\\pipe\\calculator_pipe\n";
}

class variable_pool: public vtrc_example::variable_pool {

    std::map<std::string, double> variables_;

    void get_variable(::google::protobuf::RpcController* controller,
                 const ::vtrc_example::number* request,
                 ::vtrc_example::number* response,
                 ::google::protobuf::Closure* done)
    {
        closure_holder done_holder(done);
        std::string n(request->name( ));

        std::cout << "Server wants variable: " << n << "\n";

        std::map<std::string, double>::const_iterator f(variables_.find(n));
        if( f == variables_.end( ) ) {
            std::ostringstream oss;
            oss << "Variable is not found: '" << n << "'";
            throw std::runtime_error( oss.str( ) );
        }
        response->set_value( f->second );
    }

public:

    void set( std::string const &name, double value )
    {
        /// don't need lock this calls;
        variables_[name] = value;
    }

};

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

    pool_pair pp(1, 1);

    /// create client
    vtrc::shared_ptr<vtrc_client> client(vtrc_client::create(pp));

    /// connect slot to 'on_ready'

    vtrc::mutex              ready_mutex;
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

    /// wait for client ready

    vtrc::unique_lock<vtrc::mutex> ready_lock(ready_mutex);
    ready_cond.wait( ready_lock, vtrc::bind( &vtrc_client::ready, client ) );

    /// add some variables

    vars->set( "pi", 3.1415926 );
    vars->set( "e",  2.7182818 );
    vars->set( "myvar", 100.100 );

    /// create calculator
    vtrc::unique_ptr<interfaces::calculator>
                                calc(interfaces::create_calculator( client ));

    try {
        std::cout << "res: " << calc->sum( "piz", calc->mul( "e", 1 ) ) << "\n";
    } catch( const std::exception& ex ) {
        std::cout << "calls failed: " << ex.what( ) << "\n";
    }

    return 0;

} catch( const std::exception &ex ) {
    std::cout << "General client error: " << ex.what( ) << "\n";
    return 2;
}
