#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "vtrc-client-tcp.h"
#include "vtrc-protocol-layer-c.h"

#include "vtrc-client.h"
#include "vtrc-bind.h"

namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace bsys = boost::system;

    struct client_tcp::impl  {

        typedef impl this_type;

        boost::asio::io_service &ios_;
        client_tcp              *parent_;
        std::vector<char>        read_buff_;

        vtrc_client             *client_;

        vtrc::shared_ptr<protocol_layer_c> protocol_;

        impl( boost::asio::io_service &ios, vtrc_client *client )
            :ios_(ios)
            ,read_buff_(4096 << 1)
            ,client_(client)
        {

        }

        boost::asio::ip::tcp::socket &sock( )
        {
            return parent_->get_socket( );
        }

        void init(  )
        {
            protocol_.reset(new client::protocol_layer_c( parent_, client_ ));
            start_reading( );
        }

        void connect( const std::string &address,
                      const std::string &service )
        {
            basio::ip::tcp::endpoint ep
                    (basio::ip::address::from_string(address),
                     boost::lexical_cast<unsigned short>(service) );
            sock( ).connect( ep );
            init( );
        }

        void on_connect( const boost::system::error_code &err,
                         common::closure_type closure, 
                         common::connection_iface_sptr parent)
        {
            if( !err ) {
                init( );
            }
            closure( err );
        }

        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type closure )
        {
            basio::ip::tcp::endpoint ep
                    (basio::ip::address::from_string(address),
                     boost::lexical_cast<unsigned short>(service) );
            sock( ).async_connect( ep,
                    vtrc::bind( &this_type::on_connect, this,
                                 basio::placeholders::error, closure,
                                 parent_->shared_from_this( )) );
        }

        void start_reading( )
        {
            sock( ).async_read_some(
                    basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                    vtrc::bind( &this_type::read_handler, this,
                         basio::placeholders::error,
                         basio::placeholders::bytes_transferred,
                         parent_->shared_from_this( ))
                );
        }

        void read_handler( const bsys::error_code &error, size_t bytes,
                           common::connection_iface_sptr parent)
        {
            if( !error ) {
                try {
                    protocol_->process_data( &read_buff_[0], bytes );
                } catch( const std::exception & /*ex*/ ) {
                    parent_->close( );
                    return;
                }
                start_reading( );
            } else {
                protocol_->on_read_error( error );
                parent_->close( );
            }
        }

        std::string prepare_for_write(const char *data, size_t len)
        {
            return protocol_->prepare_data( data, len );
        }

        common::protocol_layer &get_protocol( )
        {
            return *protocol_;
        }

        void on_write_error( const boost::system::error_code &err )
        {
            protocol_->on_write_error( err );
        }

    };

    client_tcp::client_tcp( boost::asio::io_service &ios, vtrc_client *client )
        :common::transport_tcp(new boost::asio::ip::tcp::socket(ios))
        ,impl_(new impl(ios, client))
    {
        impl_->parent_ = this;
    }

    vtrc::shared_ptr<client_tcp> client_tcp::create(basio::io_service &ios,
                                                        vtrc_client *client)
    {
        vtrc::shared_ptr<client_tcp> new_inst (new client_tcp( ios, client ));
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
                                    vtrc::function <
                                        void (const boost::system::error_code &)
                                    >   closure )
    {
        impl_->async_connect( address, service, closure );
    }

    void client_tcp::on_write_error( const boost::system::error_code &err )
    {
        impl_->on_write_error( err );
        close( );
    }

    common::protocol_layer &client_tcp::get_protocol( )
    {
        return impl_->get_protocol( );
    }

    std::string client_tcp::prepare_for_write(const char *data, size_t len)
    {
        return impl_->prepare_for_write( data, len );
    }

    void client_tcp::init( )
    {
    }

}}

