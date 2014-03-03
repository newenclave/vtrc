
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/atomic.hpp>

#include <deque>
#include <string>

#include "vtrc-transport-tcp.h"
#include "vtrc-enviroment.h"

namespace vtrc { namespace common {

    namespace basio = boost::asio;
    namespace bip = boost::asio::ip;
    namespace bsys = boost::system;

    struct transport_tcp::impl {

        typedef impl this_type;

        struct message_holder {
            std::string message_;
            // boost::shared_ptr<closure_type> closure_;
        };

        boost::shared_ptr<bip::tcp::socket> sock_;
        basio::io_service                  &ios_;
        enviroment                          env_;

        basio::io_service::strand           write_dispatcher_;
        std::deque<message_holder>          write_queue_;

        transport_tcp                       *parent_;
        boost::atomic_bool                   closed_;

        impl( bip::tcp::socket *s )
            :sock_(s)
            ,ios_(sock_->get_io_service( ))
            ,write_dispatcher_(ios_)
            ,closed_(false)
        {}

        const char *name( ) const
        {
            return "tcp";
        }

        void close( )
        {
            closed_ = true;
            sock_->close( );
        }

        bool active( ) const
        {
            return !closed_;
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
                                std::string( data, data + length ),
                                boost::shared_ptr<closure_type>()));
        }

        void write(const char *data, size_t length,
                                  closure_type &success)
        {
            boost::shared_ptr<closure_type>
                    closure(boost::make_shared<closure_type>(success));

            write_dispatcher_.post(
                   boost::bind( &this_type::write_impl, this,
                                std::string( data, data + length ),
                                closure));
        }

        std::string prepare_for_write( const char *data, size_t length)
        {
            return std::string( data, data + length );
        }

        void async_write( )
        {
            try {
                sock_->async_send(
                        basio::buffer( write_queue_.front( ).message_ ),
                        write_dispatcher_.wrap(
                                boost::bind( &this_type::write_handler, this,
                                     basio::placeholders::error,
                                     basio::placeholders::bytes_transferred,
                                     1))
                        );
            } catch( const std::exception & ) {
                close( );
            }

        }

        void write_impl( const std::string data,
                         boost::shared_ptr<closure_type> closure )
        {
            bool empty = write_queue_.empty( );

            message_holder mh;

            mh.message_ = parent_->prepare_for_write( data.c_str( ),
                                                      data.size( ));

            write_queue_.push_back( mh );

            if( closure.get( ) ) {
                (*closure)( boost::system::error_code( ) );
            }

            if( empty ) {
                async_write( );
            }
        }

        void write_handler( const bsys::error_code &error,
                            size_t /*bytes*/,
                            size_t /*messages*/ )
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

        boost::asio::io_service::strand &get_write_dispatcher( )
        {

            return write_dispatcher_;
        }

    };

    transport_tcp::transport_tcp( bip::tcp::socket *s )
        :impl_(new impl(s))
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

    bool transport_tcp::active( ) const
    {
        return impl_->active( );
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

    void transport_tcp::write(const char *data, size_t length,
                              closure_type &success)
    {
        return impl_->write( data, length, success );
    }

    void transport_tcp::send_message( const char *data, size_t length )
    {
        return impl_->write( data, length );
    }

    std::string transport_tcp::prepare_for_write(const char *data, size_t len)
    {
        return impl_->prepare_for_write( data, len );
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
