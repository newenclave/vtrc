#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "vtrc-client-tcp.h"

namespace vtrc { namespace client {

    namespace ba = boost::asio;

    struct client_tcp::client_tcp_impl  {
        boost::asio::io_service &ios_;
        client_tcp *parent_;

        client_tcp_impl( boost::asio::io_service &ios )
            :ios_(ios)
        { }

        boost::asio::ip::tcp::socket &sock( )
        {
            return parent_->get_socket( );
        }

        void connect( const std::string &address,
                      const std::string &service )
        {
            ba::ip::tcp::endpoint ep(ba::ip::address::from_string(address),
                    boost::lexical_cast<unsigned short>(service) );
            sock( ).connect( ep );
        }

        void async_connect( const std::string &address,
                            const std::string &service,
                            boost::function <
                                    void (const boost::system::error_code &)
                                >   closure )
        {
            ba::ip::tcp::endpoint ep(ba::ip::address::from_string(address),
                    boost::lexical_cast<unsigned short>(service) );
            sock( ).async_connect( ep, closure );
        }

        void send_message( const char *data, size_t length )
        {

        }

    };

    client_tcp::client_tcp( boost::asio::io_service &ios )
        :common::transport_tcp(new boost::asio::ip::tcp::socket(ios))
        ,impl_(new client_tcp_impl(ios))
    {
        impl_->parent_ = this;
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

    void client_tcp::send_message( const char *data, size_t length )
    {
        impl_->send_message( data, length );
    }


}}

