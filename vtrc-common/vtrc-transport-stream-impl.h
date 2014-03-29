#ifndef VTRC_TRANSPORT_STREAM_IMPL_H
#define VTRC_TRANSPORT_STREAM_IMPL_H

#include <boost/asio.hpp>

#include <deque>
#include <string>

#include "vtrc-enviroment.h"
#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-atomic.h"
#include "vtrc-mutex.h"

#include "vtrc-chrono.h"

namespace vtrc { namespace common {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;

#define TRANSPORT_USE_ASYNC_WRITE 1

    namespace {

        template<typename StreamType, typename ParentType>
        struct transport_impl {

            typedef StreamType stream_type;
            typedef ParentType parent_type;

            typedef transport_impl<stream_type, parent_type> this_type;

            struct message_holder {
                std::string message_;
                vtrc::shared_ptr<closure_type> closure_;
                message_holder( )
                { }
            };

            typedef vtrc::shared_ptr<message_holder> message_holder_sptr;

            vtrc::shared_ptr<stream_type>       sock_;
            basio::io_service                  &ios_;
            enviroment                          env_;

            parent_type                         *parent_;
            vtrc::atomic<bool>                   closed_;

            std::string                          name_;

#ifndef TRANSPORT_USE_ASYNC_WRITE
            vtrc::mutex                          write_lock_;
#else
            basio::io_service::strand            write_dispatcher_;
            std::deque<message_holder_sptr>      write_queue_;

#endif

            transport_impl( stream_type *s, const std::string &n )
                :sock_(s)
                ,ios_(sock_->get_io_service( ))
                ,write_dispatcher_(ios_)
                ,closed_(false)
                ,name_(n)
            { }

            void set_parent(parent_type *parent)
            {
                parent_ = parent;
            }

            parent_type *get_parent( )
            {
                return parent_;
            }

            virtual ~transport_impl( )
            { }

            const char *name( ) const
            {
                return name_.c_str( );
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

            virtual std::string prepare_for_write(const char *data,
                                                  size_t len) = 0;

            message_holder_sptr make_holder( const char *data, size_t length,
                                         vtrc::shared_ptr<closure_type> closure)
            {
                message_holder_sptr mh(vtrc::make_shared<message_holder>());
                mh->message_ = prepare_for_write( data, length );
                mh->closure_ = closure;
                return mh;
            }

            message_holder_sptr make_holder( const char *data, size_t length)
            {
                return make_holder(data, length,
                                            vtrc::shared_ptr<closure_type>( ));
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
                    this->close( );
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
            stream_type &get_socket( )
            {
                return *sock_;
            }

            const stream_type &get_socket( ) const
            {
                return *sock_;
            }

            boost::asio::io_service::strand &get_dispatcher( )
            {
                return write_dispatcher_;
            }
        };
    }

}}

#endif // VTRCTRANSPORTSTREAMIMPL_H