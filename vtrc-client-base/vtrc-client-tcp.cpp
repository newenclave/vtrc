#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include "vtrc-client-tcp.h"
#include "vtrc-protocol-layer-c.h"

#include "vtrc-client.h"

namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace bsys = boost::system;

    struct client_tcp::client_tcp_impl  {

        typedef client_tcp_impl this_type;

        boost::asio::io_service &ios_;
        client_tcp              *parent_;
        std::vector<char>        read_buff_;

        vtrc_client             *client_;

        boost::shared_ptr<protocol_layer> protocol_;

        client_tcp_impl( boost::asio::io_service &ios, vtrc_client *client )
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
            protocol_.reset(new client::protocol_layer( parent_ ));
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
                         common::closure_type closure )
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
                    boost::bind( &this_type::on_connect, this,
                                 basio::placeholders::error, closure) );
        }

        void start_reading( )
        {
            sock( ).async_read_some(
                    basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                    boost::bind( &this_type::read_handler, this,
                         basio::placeholders::error,
                         basio::placeholders::bytes_transferred)
                );
        }

        void read_handler( const bsys::error_code &error, size_t bytes )
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
                //std::cout << "error close\n";
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

    };

    client_tcp::client_tcp( boost::asio::io_service &ios, vtrc_client *client )
        :common::transport_tcp(new boost::asio::ip::tcp::socket(ios))
        ,impl_(new client_tcp_impl(ios, client))
    {
        impl_->parent_ = this;
    }

    boost::shared_ptr<client_tcp> client_tcp::create(basio::io_service &ios,
                                                        vtrc_client *client)
    {
        boost::shared_ptr<client_tcp> new_inst (new client_tcp( ios, client ));
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
                                    boost::function <
                                    void (const boost::system::error_code &)
                                    >   closure )
    {
        impl_->async_connect( address, service, closure );
    }

    void client_tcp::on_write_error( const boost::system::error_code &err )
    {
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

}}

