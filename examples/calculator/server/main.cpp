#include <iostream>

#include "google/protobuf/descriptor.h"

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-channels.h"

#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"

#include "protocol/calculator.pb.h"

#include "vtrc-listener-tcp.h"
#include "vtrc-listener-unix-local.h"
#include "vtrc-listener-win-pipe.h"

#include "vtrc-memory.h"
#include "vtrc-bind.h"

#include <math.h>
#include <boost/lexical_cast.hpp>

#include <stdarg.h>

using namespace vtrc::server;
using namespace vtrc::common;

/// calculator implementation
class calculator_impl: public vtrc_example::calculator {

    application      &app_;
    connection_iface *client_;
    vtrc::unique_ptr<rpc_channel> client_channel_;

    static rpc_channel *client_channel( connection_iface *client )
    {
        /// create CALLBACK channel.
        /// so, all calls will be made in client-call thread context
        return channels
                ::unicast
                ::create_callback_channel(client->shared_from_this( ));
    }

public:

    calculator_impl( application &app, connection_iface *client )
        :app_(app)
        ,client_(client)
        ,client_channel_(client_channel(client_))
    { }

private:

    typedef std::pair<double, double> number_pair;

    double request_client_variable( const std::string &name ) const
    {
        vtrc_example::variable_pool_Stub stub(client_channel_.get( ));
        vtrc_example::number num;
        num.set_name( name );
        stub.get_variable( NULL, &num, &num, NULL );
        return num.value( );
    }

    /// return value if it is set;
    /// else try to get value from client by variable 'name'
    number_pair extract_num_pair( const vtrc_example::number_pair* req ) const
    {
        number_pair result;

        if( req->first( ).has_value( ) ) result.first = req->first( ).value( );
        else result.first = request_client_variable( req->first( ).name( ) );

        if( req->second( ).has_value( )) result.second = req->second( ).value();
        else result.second = request_client_variable( req->second( ).name( ) );

        return result;
    }

    void sum(::google::protobuf::RpcController* /*controller*/,
             const ::vtrc_example::number_pair* request,
             ::vtrc_example::number* response,
             ::google::protobuf::Closure* done)
    {
        closure_holder done_holder( done );

        number_pair req = extract_num_pair( request );
        response->set_value( req.first + req.second );

    }
    void mul(::google::protobuf::RpcController* /*controller*/,
             const ::vtrc_example::number_pair* request,
             ::vtrc_example::number* response,
             ::google::protobuf::Closure* done)
    {
        closure_holder done_holder( done );
        number_pair req = extract_num_pair( request );
        response->set_value( req.first * req.second );
    }

    void div(::google::protobuf::RpcController* /*controller*/,
             const ::vtrc_example::number_pair* request,
             ::vtrc_example::number* response,
             ::google::protobuf::Closure* done)
    {
        closure_holder done_holder( done );
        number_pair req = extract_num_pair( request );

        if( req.second == 0 )
            throw std::logic_error( "Division by zero." );

        response->set_value( req.first / req.second );

    }

    void pow(::google::protobuf::RpcController* /*controller*/,
             const ::vtrc_example::number_pair* request,
             ::vtrc_example::number* response,
             ::google::protobuf::Closure* done)
    {
        closure_holder done_holder( done );
        number_pair req = extract_num_pair( request );
        response->set_value( ::pow(req.first, req.second) );
    }
};

/// just in case; rpc_service_wrapper can be used instead
class my_rpc_wrapper: public rpc_service_wrapper
{
public:
    my_rpc_wrapper( google::protobuf::Service *my_serv )
        :rpc_service_wrapper(my_serv)
    { }
};

class server_application: public application {

    typedef server_application this_type;

public:

    server_application( pool_pair &pair )
        :application( pair )
    { }

    vtrc::shared_ptr<rpc_service_wrapper>
             get_service_by_name( connection_iface* connection,
                                  const std::string &service_name )
    {
        static const std::string calculator_name =
                calculator_impl::descriptor( )->full_name( );

        if( calculator_name == service_name )
            return vtrc::make_shared<my_rpc_wrapper>(
                                    new calculator_impl( *this, connection ) );

        return vtrc::shared_ptr<my_rpc_wrapper>( );
    }

    void on_new_connection( listener_sptr l, const connection_iface *c )
    {
        std::cout << "New connection: "
                  << "ep: "     << l->name( ) << "; "
                  << "client: " << c->name( )
                  << "\n"
                    ;

        /// here we cat stop listener without any problem
        //l->stop( );

    }

    void on_stop_connection( listener_sptr l, const connection_iface *c )
    {
        std::cout << "Close connection: " << c->name( ) << "\n";

        /// yes, we can stop it and start again.
        //l->start( );
    }

    void attach_listener( listener_sptr listen )
    {
        listen->get_on_new_connection( ).connect(
                    vtrc::bind( &this_type::on_new_connection, this,
                                listen, _1 ));
        listen->get_on_stop_connection( ).connect(
                    vtrc::bind( &this_type::on_stop_connection, this,
                                listen, _1 ));
    }
};

void usage(  )
{
    std::cout
          << "Usage: \n\tcalculator_server <server ip> <server port>\n"
          << "or:\n\tcalculator_server <localname>\n"
          << "examples:\n"
          << "\tfor tcp: calculator_server 0.0.0.0 55555\n"
          << "\tfor unix: calculator_server ~/calculator.sock\n"
          << "\tfor win pipe: calculator_server \\\\.\\pipe\\calculator_pipe\n";
}

int main( int argc, char **argv ) try
{
    if( argc < 2 ) {
        usage( );
        return 1;
    }

    /// one thread for transport io, and one for calls
    pool_pair pp(1, 1);
    server_application app( pp );

    vtrc::shared_ptr<listener> main_listener;

    if( argc < 3 ) {
#ifndef _WIN32
        ::unlink( argv[1] );
        main_listener = listeners::unix_local::create(app, argv[1]);
#else
        main_listener = listeners::win_pipe::create(app, argv[1]);
#endif
    } else {
        main_listener = listeners::tcp::create(app, argv[1],
                boost::lexical_cast<unsigned short>(argv[2]));
    }

    app.attach_listener( main_listener );

    std::cout << "starting '" << main_listener->name( ) << "'...";

    main_listener->start( );

    std::cout << "success\n";

    pp.join_all( );

    return 0;

} catch( const std::exception &ex ) {
    std::cout << "General server error: " << ex.what( ) << "\n";
    return 2;
}

