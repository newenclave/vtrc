#include <iostream>

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-tcp.h"
#include "vtrc-server/vtrc-channels.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-mutex.h"
#include "vtrc-bind.h"

#include "protocol/proxy-calls.pb.h"   /// hello protocol
#include "google/protobuf/descriptor.h" /// for descriptor( )->full_name( )
#include "boost/lexical_cast.hpp"


using namespace vtrc;

namespace {

class proxy_application;

class transmitter_impl: public proxy::transmitter {

    proxy_application         *app_;
    common::connection_iface  *connection_;

public:

    transmitter_impl( proxy_application *app, common::connection_iface *c )
        :app_(app)
        ,connection_(c)
    { }

    static std::string const &service_name(  )
    {
        return proxy::transmitter::descriptor( )->full_name( );
    }

private:

    common::connection_iface_sptr client_by_id( const std::string &id ) const;

    void register_client( );

    void send_to( ::google::protobuf::RpcController* /*controller*/,
                  const ::proxy::proxy_message* request,
                  ::proxy::proxy_message* response,
                  ::google::protobuf::Closure* done )
    {
        common::closure_holder holder( done );
        common::connection_iface_sptr tgt =
                                       client_by_id( request->client_id( ) );

        vtrc::shared_ptr<rpc::lowlevel_unit> res;

        if( tgt ) {

            vtrc::shared_ptr<rpc::lowlevel_unit> llu(new rpc::lowlevel_unit);

            llu->ParseFromString( request->data( ) );

            vtrc::unique_ptr<common::rpc_channel> chan(
                        server::channels::unicast::create( tgt ));

            llu->set_channel_data( connection_->name( ) ) ;

            std::cout << "set_channel_id " << chan->channel_data( ) << "\n";

            res = chan->raw_call( llu, request,
                                  common::lowlevel_closure_type( ) );

        } else {
            res.reset( new rpc::lowlevel_unit );
            res->mutable_error( )->set_code( 1 );
            res->mutable_error( )->set_additional( "Target channel is empty." );
        }
        response->set_data( res->SerializeAsString( ) );
    }

    void reg_for_proxy(::google::protobuf::RpcController* controller,
             const ::proxy::empty* request,
             ::proxy::empty* response,
             ::google::protobuf::Closure* done)
    {
        common::closure_holder holder( done );
        register_client( );
    }
};

class proxy_application: public server::application {

    typedef common::rpc_service_wrapper     wrapper_type;
    typedef vtrc::shared_ptr<wrapper_type>  wrapper_sptr;
    typedef std::map<std::string, common::connection_iface_wptr> clients_map;

    clients_map  clients_;
    vtrc::mutex  clients_lock_;

public:

    proxy_application( common::pool_pair &pp )
        :server::application(pp)
    { }

    void register_client( common::connection_iface* c,
                          const std::string &id )
    {
        vtrc::unique_lock<vtrc::mutex> lck(clients_lock_);
        clients_[id] = c->weak_from_this( );
    }

    common::connection_iface_sptr client_by_id( const std::string &id )
    {
        vtrc::unique_lock<vtrc::mutex> lck(clients_lock_);
        clients_map::iterator f(clients_.find( id ));
        common::connection_iface_sptr res;
        if( f != clients_.end( ) ) {
            res = f->second.lock( );
            if( !res ) {
                clients_.erase( f );
            }
        }
        return res;
    }

    wrapper_sptr get_service_by_name( common::connection_iface* connection,
                                      const std::string &service_name )
    {
        if( service_name == transmitter_impl::service_name( ) ) {

             transmitter_impl *new_impl =
                     new transmitter_impl( this, connection );

             return vtrc::make_shared<wrapper_type>( new_impl );

        }
        return wrapper_sptr( );
    }

};

common::connection_iface_sptr transmitter_impl::client_by_id(
                                                const std::string &id  ) const
{
    return app_->client_by_id( id );
}

void transmitter_impl::register_client( )
{
    app_->register_client( connection_, connection_->name( ) );
}

void new_connection( const common::connection_iface *c, proxy_application *app )
{
    std::cout << "+" << c->name( ) << std::endl;
}

void del_connection( const common::connection_iface *c )
{
    std::cout << "-" << c->name( )  << std::endl;
}

} // namespace

int main( int argc, const char **argv )
{

    const char *address = "127.0.0.1";
    unsigned short port = 56560;

    common::pool_pair pp( 0, 1 );
    proxy_application app( pp );

    try {

        if( argc > 2 ) {
            address = argv[1];
            port = boost::lexical_cast<unsigned short>( argv[2] );
        } else if( argc > 1 ) {
            port = boost::lexical_cast<unsigned short>( argv[1] );
        }

        vtrc::shared_ptr<server::listener>
                tcp( server::listeners::tcp::create( app, address, port ) );

        tcp->on_new_connection_connect(
                    vtrc::bind(new_connection, vtrc::placeholders::_1, &app) );

        tcp->on_stop_connection_connect( del_connection );

        tcp->start( );

        pp.get_io_pool( ).attach( );

    } catch( const std::exception &ex ) {
        std::cerr << "Hello, world failed: " << ex.what( ) << "\n";
    }

    pp.join_all( );

    /// make valgrind happy.
    google::protobuf::ShutdownProtobufLibrary( );

    return 0;
}

