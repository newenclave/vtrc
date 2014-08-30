
#include "boost/lexical_cast.hpp"

#include "vtrc-client-tcp.h"
#include "vtrc-client-stream-impl.h"

namespace vtrc { namespace client {

    namespace {
        typedef boost::asio::ip::tcp::socket socket_type;
        typedef client_stream_impl<client_tcp, socket_type> super_type;
    }

    struct client_tcp::impl: public super_type {

        typedef impl this_type;

        bool no_delay_;

        impl( boost::asio::io_service &ios, vtrc_client *client, bool nodelay )
            :super_type(ios, client, 4096)
            ,no_delay_(nodelay)
        { }

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
            if( no_delay_ )
                get_parent( )->set_no_delay( true );
        }

        void async_connect( const std::string &address,
                            unsigned short     service,
                            common::system_closure_type closure )
        {
            basio::ip::tcp::endpoint ep
                    (basio::ip::address::from_string(address), service );
            get_socket( ).async_connect( ep,
                    vtrc::bind( &this_type::on_connect, this,
                                 basio::placeholders::error, closure,
                                 parent_->shared_from_this( )) );
        }
    };

    static vtrc::shared_ptr<socket_type> create_socket( basio::io_service &ios )
    {
        return vtrc::make_shared<socket_type>(vtrc::ref(ios));
    }

    client_tcp::client_tcp(boost::asio::io_service &ios,
                            vtrc_client *client, bool tcp_nodelay)
        :common::transport_tcp(create_socket(ios))
        ,impl_(new impl(ios, client, tcp_nodelay ))
    {
        impl_->set_parent( this );
    }

    vtrc::shared_ptr<client_tcp> client_tcp::create(basio::io_service &ios,
                                        vtrc_client *client, bool tcp_nodelay)
    {
        vtrc::shared_ptr<client_tcp> new_inst
                    (new client_tcp( ios, client, tcp_nodelay ));
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

    void client_tcp::on_write_error( const boost::system::error_code &err )
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

    common::enviroment &client_tcp::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

    void client_tcp::on_close( )
    {
        impl_->on_close( );
    }

    std::string client_tcp::prepare_for_write(const char *data, size_t len)
    {
        return impl_->prepare_for_write( data, len );
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

}}

