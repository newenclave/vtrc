
#include "boost/lexical_cast.hpp"

#include "vtrc-client-tcp.h"
#include "vtrc-client-stream-impl.h"

namespace vtrc { namespace client {

    namespace {
        typedef VTRC_ASIO::ip::tcp::socket socket_type;
        typedef client_stream_impl<client_tcp, socket_type> super_type;
    }

    struct client_tcp::impl: public super_type {

        typedef impl this_type;

        bool no_delay_;
        std::string name_;

        impl( VTRC_ASIO::io_service &ios,
              client::base *client, protocol_signals *callbacks,
              bool nodelay )
            :super_type(ios, client, callbacks, 4096)
            ,no_delay_(nodelay)
            ,name_("tcp://<unknown>")
        { }

        void make_name( const std::string &address,
                               unsigned short     service )
        {
            std::ostringstream oss;
            oss << "tcp://" << address << ":" << service;
            name_ = oss.str( );
        }

        void connect( const std::string &address,
                      unsigned short     service )
        {
            basio::ip::tcp::endpoint ep
                    (basio::ip::address::from_string(address), service );
            get_socket( ).connect( ep );
            connection_setup( );
            start_reading( );
        }

        void connection_setup( )
        {
            make_name(
                get_socket( ).local_endpoint( ).address( ).to_string( ),
                get_socket( ).local_endpoint( ).port( ) );

            if( no_delay_ ) {
                get_parent( )->set_no_delay( true );
            }
        }

        void async_connect( const std::string &address,
                            unsigned short     service,
                            common::system_closure_type closure )
        {
            basio::ip::tcp::endpoint ep
                    (basio::ip::address::from_string(address), service );

            get_socket( ).async_connect( ep,
                    vtrc::bind( &this_type::on_connect, this,
                                 vtrc::placeholders::error, closure,
                                 parent_->shared_from_this( )) );
        }
    };

    static vtrc::shared_ptr<socket_type> create_socket( basio::io_service &ios )
    {
        return vtrc::make_shared<socket_type>(vtrc::ref(ios));
    }

    client_tcp::client_tcp(VTRC_ASIO::io_service &ios,
                           client::base *client,
                           protocol_signals *callbacks,
                           bool tcp_nodelay)
        :common::transport_tcp(create_socket(ios))
        ,impl_(new impl(ios, client, callbacks, tcp_nodelay ))
    {
        impl_->set_parent( this );
    }

    vtrc::shared_ptr<client_tcp> client_tcp::create( client::base *client,
                                        protocol_signals *callbacks,
                                        bool tcp_nodelay)
    {
        vtrc::shared_ptr<client_tcp> new_inst
                    (new client_tcp( client->get_io_service( ), client,
                                     callbacks, tcp_nodelay ));
        new_inst->init( );
        return new_inst;
    }

    client_tcp::~client_tcp( )
    {
        delete impl_;
    }

    void client_tcp::connect( const std::string &address,
                              unsigned short service )
    {
        impl_->connect( address, service );
    }

    void client_tcp::async_connect(const std::string &address,
                                   unsigned short service,
                                   common::system_closure_type closure )
    {
        impl_->async_connect( address, service, closure );
    }

    void client_tcp::on_write_error( const VTRC_SYSTEM::error_code &err )
    {
        impl_->on_write_error( err );
        close( );
    }

    const common::call_context *client_tcp::get_call_context( ) const
    {
        return impl_->get_call_context( );
    }

    common::protocol_layer &client_tcp::get_protocol( )
    {
        return impl_->get_protocol( );
    }

    const common::protocol_layer &client_tcp::get_protocol( ) const
    {
        return impl_->get_protocol( );
    }

    common::enviroment &client_tcp::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

    void client_tcp::on_close( )
    {
        impl_->on_close( );
    }

    void client_tcp::init( )
    {
        impl_->init( );
    }

    bool client_tcp::active( ) const
    {
        return impl_->active( );
    }

    const std::string &client_tcp::id( ) const
    {
        return impl_->id( );
    }

    std::string client_tcp::name( ) const
    {
        return impl_->name_;
    }

    void client_tcp::raw_call_local (
                          vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                          common::empty_closure_type done )
    {
        impl_->raw_call_local( ll_mess, done );
    }

}}

