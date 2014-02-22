
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <deque>
#include <string>

#include "vtrc-transport-tcp.h"
#include "vtrc-enviroment.h"

namespace vtrc { namespace common {

    namespace basio = boost::asio;
    namespace bip = boost::asio::ip;
    namespace bsys = boost::system;

    struct transport_tcp::transport_tcp_impl {

        typedef transport_tcp_impl this_type;

        boost::shared_ptr<bip::tcp::socket> sock_;
        basio::io_service                  &ios_;
        enviroment                          env_;

        basio::io_service::strand           write_dispatcher_;
        std::deque<std::string>             write_queue_;

        transport_tcp                       *parent_;

        transport_tcp_impl( bip::tcp::socket *s )
            :sock_(s)
            ,ios_(sock_->get_io_service( ))
            ,write_dispatcher_(ios_)
        {}

        const char *name( ) const
        {
            return "tcp";
        }

        void close( )
        {
            sock_->close( );
        }

        common::enviroment &get_enviroment( )
        {
            return env_;
        }

        boost::asio::io_service &get_io_service( )
        {
            return ios_;
        }

        void write( const char *data, size_t length )
        {
            write_dispatcher_.post(
                   boost::bind( &this_type::write_impl, this,
                                std::string( data, data + length )));
        }

        std::string prepare_for_write( const char * /*data*/, size_t /*length*/)
        { }

        void async_write( )
        {
            try {
                sock_->async_send(
                        basio::buffer( write_queue_.front( ) ),
                        write_dispatcher_.wrap(
                                boost::bind( &this_type::write_handler, this,
                                     basio::placeholders::error,
                                     basio::placeholders::bytes_transferred ))
                        );
            } catch( const std::exception & ) {
                close( );
            }
        }

        void write_impl( const std::string data )
        {
            bool empty = write_queue_.empty( );

            write_queue_.push_back( parent_->prepare_for_write( data.c_str( ),
                                                                data.size( )) );

            if( empty ) {
                async_write( );
            }
        }

        void write_handler( const bsys::error_code &error, size_t bytes )
        {
            if( !error ) {
                write_queue_.pop_front( );
                if( !write_queue_.empty( ) )
                    async_write( );
            } else {
                parent_->on_write_error( error );
            }
        }

        boost::asio::ip::tcp::socket &get_socket( )
        {
            return *sock_;
        }

        const boost::asio::ip::tcp::socket &get_socket( ) const
        {
            return *sock_;
        }
    };

    transport_tcp::transport_tcp( bip::tcp::socket *s )
        :impl_(new transport_tcp_impl(s))
    {
        impl_->parent_ = this;
    }

    transport_tcp::~transport_tcp(  )
    {
        delete impl_;
    }

    const char *transport_tcp::name( ) const
    {
        return impl_->name( );
    }

    void transport_tcp::close( )
    {
        impl_->close( );
    }

    common::enviroment &transport_tcp::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

    boost::asio::io_service &transport_tcp::get_io_service( )
    {
        return impl_->get_io_service( );
    }

    void transport_tcp::write( const char *data, size_t length )
    {
        return impl_->write( data, length );
    }

    std::string transport_tcp::prepare_for_write(const char *data, size_t len)
    {

    }

    boost::asio::ip::tcp::socket &transport_tcp::get_socket( )
    {
        return impl_->get_socket( );
    }

    const boost::asio::ip::tcp::socket &transport_tcp::get_socket( ) const
    {
        return impl_->get_socket( );
    }

}}
