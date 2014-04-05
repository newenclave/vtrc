
#include <boost/lexical_cast.hpp>

#include "vtrc-client-tcp.h"
#include "vtrc-client-stream-impl.h"

namespace vtrc { namespace client {

    namespace {
        typedef boost::asio::ip::tcp::socket socket_type;
        typedef client_stream_impl<client_tcp, socket_type> super_type;
    }

    struct client_tcp::impl: public super_type {

        typedef impl this_type;

        impl( boost::asio::io_service &ios, vtrc_client *client )
            :super_type(ios, client, 4096)
        { }

        void connect( const std::string &address,
                      const std::string &service )
        {
            basio::ip::tcp::endpoint ep
                    (basio::ip::address::from_string(address),
                     boost::lexical_cast<unsigned short>(service) );
            get_socket( ).connect( ep );
            start_reading( );
        }

        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type closure )
        {
            basio::ip::tcp::endpoint ep
                    (basio::ip::address::from_string(address),
                     boost::lexical_cast<unsigned short>(service) );
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

    client_tcp::client_tcp( boost::asio::io_service &ios, vtrc_client *client )
        :common::transport_tcp(create_socket(ios))
        ,impl_(new impl(ios, client))
    {
        impl_->set_parent( this );
    }

    vtrc::shared_ptr<client_tcp> client_tcp::create(basio::io_service &ios,
                                                        vtrc_client *client)
    {
        vtrc::shared_ptr<client_tcp> new_inst (new client_tcp( ios, client ));
        new_inst->init( );
        return new_inst;
    }

    client_tcp::~client_tcp( )
    {
        delete impl_;
    }

    void client_tcp::connect( const std::string &address,
                              const std::string &service )
    {
        impl_->connect( address, service );
    }

    void client_tcp::async_connect( const std::string &address,
                                    const std::string &service,
                                    common::closure_type closure )
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

}}

