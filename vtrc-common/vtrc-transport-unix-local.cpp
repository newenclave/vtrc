#include "vtrc-transport-unix-local.h"

#ifndef  _WIN32


#include <boost/asio.hpp>

#include <deque>
#include <string>

#include "vtrc-transport-tcp.h"
#include "vtrc-enviroment.h"
#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-atomic.h"
#include "vtrc-mutex.h"

#include "vtrc-chrono.h"

#define TRANSPORT_USE_ASYNC_WRITE 1

namespace vtrc { namespace common {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;

    struct transport_unix_local::impl {

        typedef impl this_type;

        typedef transport_unix_local::socket_type socket_type;

        struct message_holder {
            std::string message_;
            vtrc::shared_ptr<closure_type> closure_;
            message_holder( )
            { }
        };

        typedef vtrc::shared_ptr<message_holder> message_holder_sptr;

        vtrc::shared_ptr<socket_type>       sock_;
        basio::io_service                  &ios_;
        enviroment                          env_;

        transport_unix_local                *parent_;
        vtrc::atomic<bool>                   closed_;

#ifndef TRANSPORT_USE_ASYNC_WRITE
        vtrc::mutex                          write_lock_;
#else
        basio::io_service::strand            write_dispatcher_;
        std::deque<message_holder_sptr>      write_queue_;

#endif

        impl( socket_type *s )
            :sock_(s)
            ,ios_(sock_->get_io_service( ))
            ,write_dispatcher_(ios_)
            ,closed_(false)
        { }

        ~impl( )
        { }

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

        message_holder_sptr make_holder( const char *data, size_t length,
                                     vtrc::shared_ptr<closure_type> closure)
        {
            message_holder_sptr mh(vtrc::make_shared<message_holder>());
            mh->message_ = parent_->prepare_for_write( data, length );
            mh->closure_ = closure;
            return mh;
        }

        message_holder_sptr make_holder( const char *data, size_t length)
        {
            return make_holder(data, length, vtrc::shared_ptr<closure_type>( ));
        }

        static void wake_up( ) { }

        void write_raw( const char *data, size_t length )
        {
#ifdef TRANSPORT_USE_ASYNC_WRITE
            //write_dispatcher_.post( &this_type::wake_up );

            message_holder_sptr mh(vtrc::make_shared<message_holder>( ));
            mh->message_.assign( data, data + length );

            write_dispatcher_.post(
                   vtrc::bind( &this_type::write_impl, this, mh,
                                vtrc::shared_ptr<closure_type>( ),
                                parent_->shared_from_this( )));

#endif
        }

        void write( const char *data, size_t length )
        {
            message_holder_sptr mh(make_holder(data, length));

#ifndef TRANSPORT_USE_ASYNC_WRITE

            vtrc::unique_lock<vtrc::mutex> l(write_lock_);
            basio::write( *sock_, basio::buffer( mh->message_ ) );
#else
            write_dispatcher_.post(
                   vtrc::bind( &this_type::write_impl, this, mh,
                                vtrc::shared_ptr<closure_type>( ),
                                parent_->shared_from_this( )));
#endif
        }

        void write(const char *data, size_t length, closure_type &success)
        {

#ifndef TRANSPORT_USE_ASYNC_WRITE
            message_holder_sptr mh(make_holder(data, length));

            vtrc::unique_lock<vtrc::mutex> l(write_lock_);
            bsys::error_code ec;
            basio::write( *sock_, basio::buffer( mh->message_ ), ec );
            success( ec );
#else
            vtrc::shared_ptr<closure_type>
                    closure(vtrc::make_shared<closure_type>(success));
            message_holder_sptr mh(make_holder(data, length, closure));

            write_dispatcher_.post(
                   vtrc::bind( &this_type::write_impl, this, mh,
                                closure, parent_->shared_from_this( )));
#endif
        }

        std::string prepare_for_write( const char *data, size_t length)
        {
            return std::string( data, data + length );
        }


#ifdef TRANSPORT_USE_ASYNC_WRITE

        void async_write( )
        {
            try {
                sock_->async_send(
                        basio::buffer( write_queue_.front( )->message_ ),
                        write_dispatcher_.wrap(
                                vtrc::bind( &this_type::write_handler, this,
                                     basio::placeholders::error,
                                     basio::placeholders::bytes_transferred,
                                     1, parent_->shared_from_this( )))
                        );
            } catch( const std::exception & ) {
                close( );
            }

        }
        void write_impl( message_holder_sptr data,
                         vtrc::shared_ptr<closure_type> closure,
                         common::connection_iface_sptr inst)
        {
            bool empty = write_queue_.empty( );

            write_queue_.push_back( data );

            if( empty ) {
                async_write( );
            }
        }

        void write_handler( const bsys::error_code &error,
                            size_t /*bytes*/,
                            size_t messages,
                            common::connection_iface_sptr /*inst*/)
        {

            if( !error ) {
                while( messages-- ) {
                    if( write_queue_.front( )->closure_ ) {
                        (*write_queue_.front( )->closure_)( error );
                    }

//                    std::cout << "mesage was queued "
//                              << vtrc::chrono::high_resolution_clock::now( ) -
//                                 write_queue_.front( )->stored_
//                              << "\n";

                    write_queue_.pop_front( );
                }
                if( !write_queue_.empty( ) )
                    async_write( );

            } else {
                while( messages-- ) {
                    if( write_queue_.front( )->closure_ ) {
                        (*write_queue_.front( )->closure_)( error );
                    }
                }
                parent_->on_write_error( error );
            }
        }
#endif

        socket_type &get_socket( )
        {
            return *sock_;
        }

        const socket_type &get_socket( ) const
        {
            return *sock_;
        }

        boost::asio::io_service::strand &get_dispatcher( )
        {
            return write_dispatcher_;
        }
    };

    transport_unix_local::transport_unix_local(
                                         transport_unix_local::socket_type *s )
        :impl_(new impl(s))
    {
        impl_->parent_ = this;
    }

    transport_unix_local::~transport_unix_local(  )
    {
        delete impl_;
    }

    const char *transport_unix_local::name( ) const
    {
        return impl_->name( );
    }

    void transport_unix_local::close( )
    {
        impl_->close( );
    }

    bool transport_unix_local::active( ) const
    {
        return impl_->active( );
    }

    common::enviroment &transport_unix_local::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

    void transport_unix_local::write( const char *data, size_t length )
    {
        return impl_->write( data, length );
    }

    void transport_unix_local::write(const char *data, size_t length,
                              closure_type &success)
    {
        impl_->write( data, length, success );
    }

    std::string transport_unix_local::prepare_for_write(const char *data,
                                                        size_t len)
    {
        return impl_->prepare_for_write( data, len );
    }

    transport_unix_local::socket_type &transport_unix_local::get_socket( )
    {
        return impl_->get_socket( );
    }

    const transport_unix_local::socket_type
                                    &transport_unix_local::get_socket( ) const
    {
        return impl_->get_socket( );
    }

}}

#endif