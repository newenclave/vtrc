#ifndef ASYNC_TRANSPORT_POINT_HPP
#define ASYNC_TRANSPORT_POINT_HPP

#include "boost/asio.hpp"

#include "boost/bind.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "boost/make_shared.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/function.hpp"

#include <string>
#include <queue>

namespace async_transport {

    template <typename ST>
    class point_iface: public boost::enable_shared_from_this<point_iface<ST> > {

        typedef point_iface<ST> this_type;

    public:

        typedef boost::shared_ptr<this_type> shared_type;
        typedef boost::weak_ptr<this_type>   weak_type;
        typedef ST stream_type;

        enum message_transform_type {
            DONT_TRANSFORM_MESSAGE,
            TRANSFORM_MESSAGE
        };

        enum read_dispacth_type {
            DONT_DISPATCH_READ,
            DISPATCH_READ
        };

        typedef boost::function <
            void (const boost::system::error_code &)
        > write_closure;

    private:

        typedef std::string message_type;

        struct queue_value {

            typedef boost::shared_ptr<queue_value> shared_type;

            message_type    message_;
            write_closure   success_;

            queue_value( const char *data, size_t length )
                :message_(data, length)
            { }

            static
            shared_type create( const char *data, size_t length )
            {
                return boost::make_shared<queue_value>( data, length );
            }
        };

        typedef typename queue_value::shared_type  queue_value_sptr;

        typedef std::queue<queue_value_sptr> message_queue_type;

        typedef void (this_type::*call_impl)( );
        typedef int  priority_type;

        boost::asio::io_service          &ios_;
        boost::asio::io_service::strand   write_dispatcher_;
        stream_type                       stream_;

        message_queue_type                write_queue_;

        std::vector<char>                 read_buffer_;
        call_impl                         read_impl_;
        call_impl                         async_write_impl_;

        bool                              active_;

        static
        call_impl get_read_dispatch( read_dispacth_type dispatch )
        {
            return ( dispatch == DISPATCH_READ )
                   ? &this_type::start_read_impl_wrap
                   : &this_type::start_read_impl;
        }

        static
        call_impl get_message_transform( message_transform_type transform )
        {
            return ( transform == TRANSFORM_MESSAGE )
                   ? &this_type::async_write_transform
                   : &this_type::async_write_no_transform;
        }

    protected:

        point_iface( boost::asio::io_service &ios,
                     size_t read_block_size,
                     read_dispacth_type dispatch_read,
                     message_transform_type transform_message )
            :ios_(ios)
            ,write_dispatcher_(ios_)
            ,stream_(ios_)
            ,read_buffer_(read_block_size)
            ,read_impl_(get_read_dispatch(dispatch_read))
            ,async_write_impl_(get_message_transform(transform_message))
            ,active_(true)
        { }

    private:

        void close_impl( shared_type /*inst*/ )
        {
            if( active_ ) {
                active_ = false;
                stream_.close( );
            }
        }

        void post_close(  )
        {
            write_dispatcher_.post(
                        boost::bind( &this_type::close_impl, this,
                                     this->shared_from_this( ) ));
        }

        /// =========== queue wrap calls =========== ///

        void queue_push( const queue_value_sptr &new_mess )
        {
            write_queue_.push( new_mess );
        }

        const queue_value_sptr &queue_top( )
        {
            return write_queue_.front( );
        }

        void queue_pop( )
        {
            write_queue_.pop( );
        }

        bool queue_empty( ) const
        {
            return write_queue_.empty( );
        }

        /// ================ write ================ ///

        void async_write( )
        {
            (this->*async_write_impl_)( );
        }

        void async_write( const char *data, size_t length, size_t total )
        {
            try {
                stream_.async_write_some(
                        boost::asio::buffer( data, length ),
                        write_dispatcher_.wrap(
                            boost::bind( &this_type::write_handler, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            length, total,
                            this->shared_from_this( ))
                        )
                );
            } catch( const std::exception & ) {
                /// generate error
                on_write_exception(  );
            }
        }

        void async_write_transform(  )
        {
            std::string &top( queue_top( )->message_ );

            top.assign( on_transform_message( top ) );

            async_write( top.c_str( ), top.size( ), 0 );
        }

        void async_write_no_transform(  )
        {
            const std::string &top( queue_top( )->message_ );
            async_write( top.c_str( ), top.size( ), 0);
        }

        void write_handler( const boost::system::error_code &error,
                            size_t const bytes,
                            size_t const length,
                            size_t       total,
                            shared_type  /*this_inst*/)
        {
            queue_value &top( *queue_top( ) );

            if( !error ) {

                if( bytes < length ) {

                    total += bytes;

                    const std::string &top_mess( top.message_ );
                    async_write(top_mess.c_str( ) + total,
                                top_mess.size( )  - total, total);

                } else {

                    if( top.success_ ) {
                        top.success_( error );
                    }

                    queue_pop( );

                    if( !queue_empty( ) ) {
                        async_write(  );
                    }

                }
            } else {
                /// generate error
                if( top.success_ ) {
                    top.success_( error );
                }
                on_write_error( error );
            }

        }

        void write_impl( const queue_value_sptr data, shared_type /*inst*/ )
        {
            const bool empty = queue_empty( );

            queue_push( data );

            if( empty ) {
                async_write(  );
            }
        }

        void post_write( const char *data, size_t len,
                         const write_closure &close )
        {
            queue_value_sptr inst(queue_value::create( data, len ));
            inst->success_ = close;

            write_dispatcher_.post(
                    boost::bind( &this_type::write_impl, this,
                                 inst, this->shared_from_this( ) )
                    );
        }

        /// ================ read ================ ///

        void async_read( )
        {
            (this->*read_impl_)( );
        }

        void read_handler( const boost::system::error_code &error,
                           size_t const bytes, shared_type /*inst*/ )
        {
            if( !error ) {
                on_read( &read_buffer_[0], bytes );
                async_read( );
            } else {
                /// genegate error;
                on_read_error( error );
            }
        }

        void start_read_impl_wrap(  )
        {
            /// use strand::wrap for callback
            stream_.async_read_some(
                    boost::asio::buffer(&read_buffer_[0], read_buffer_.size( )),
                    write_dispatcher_.wrap(
                        boost::bind( &this_type::read_handler, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred,
                        this->shared_from_this( ))
                    )
             );
        }

        void start_read_impl(  )
        {
            stream_.async_read_some(
                    boost::asio::buffer(&read_buffer_[0], read_buffer_.size( )),
                    boost::bind( &this_type::read_handler, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred,
                        this->shared_from_this( )
                    )
            );
        }

    private:

        virtual void on_read( const char *data, size_t length ) = 0;

        virtual void on_read_error( const boost::system::error_code &code )
        { }

        virtual void on_write_error( const boost::system::error_code &code )
        { }

        virtual void on_write_exception(  )
        {
            throw;
        }

        virtual std::string on_transform_message( std::string &message )
        {
            return message;
        }

    public:

        virtual ~point_iface( ) { }

        boost::asio::io_service &get_io_service( )
        {
            return ios_;
        }

        const boost::asio::io_service &get_io_service( ) const
        {
            return ios_;
        }

        const boost::asio::io_service::strand &get_dispatcher( ) const
        {
            return write_dispatcher_;
        }

        boost::asio::io_service::strand &get_dispatcher( )
        {
            return write_dispatcher_;
        }

        stream_type &get_stream( )
        {
            return stream_;
        }

        const stream_type &get_stream( ) const
        {
            return stream_;
        }

        void write( const std::string &data )
        {
            write_post_notify( data.c_str( ), data.size( ), write_closure( ) );
        }

        void write_post_notify( const std::string &data,
                                const write_closure &closuse )
        {
            write_post_notify( data.c_str( ), data.size( ), closuse );
        }

        void write( const char *data, size_t length )
        {
            post_write( data, length, write_closure( ) );
        }

        void write_post_notify( const char *data, size_t length,
                                const write_closure &closuse )
        {
            post_write( data, length, closuse );
        }

        void start_read( )
        {
            async_read( );
        }

        void close( )
        {
            post_close( );
        }

    };

}


#endif // ASYNC_TRANSPORT_POINT_HPP
