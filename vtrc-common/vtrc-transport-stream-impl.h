#ifndef VTRC_TRANSPORT_STREAM_IMPL_H
#define VTRC_TRANSPORT_STREAM_IMPL_H


#include <deque>
#include <string>

#include "boost/asio.hpp"

#include "vtrc-enviroment.h"
#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-mutex.h"
#include "vtrc-closure.h"

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
                vtrc::shared_ptr<system_closure_type> closure_;
                bool on_send_;
                message_holder( )
                { }
            };

            typedef vtrc::shared_ptr<message_holder> message_holder_sptr;

            vtrc::shared_ptr<stream_type>       stream_;
            basio::io_service                  &ios_;
            basio::io_service::strand           write_dispatcher_;

            enviroment                          env_;

            parent_type                         *parent_;
            bool                                 closed_;
            vtrc::mutex                          close_lock_;

            std::string                          name_;


#ifndef TRANSPORT_USE_ASYNC_WRITE
            vtrc::mutex                          write_lock_;
#else
            std::deque<message_holder_sptr>      write_queue_;

#endif

            transport_impl( vtrc::shared_ptr<stream_type> s,
                            const std::string &n )
                :stream_(s)
                ,ios_(stream_->get_io_service( ))
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

            const parent_type *get_parent( ) const
            {
                return parent_;
            }

            virtual ~transport_impl( )
            { }

            std::string name( ) const
            {
                std::ostringstream oss;
                oss << name_ << "://<unknown>";
                return oss.str( );
            }

            void close( )
            {
                vtrc::lock_guard<vtrc::mutex> lk(close_lock_);
                if( !closed_ ) {
                    closed_ = true;
                    stream_->close( );
                    parent_->on_close( );
                }
            }

            bool closed( ) const
            {
                return closed_;
            }

            boost::asio::io_service &get_io_service( )
            {
                return ios_;
            }

            void prepare_for_write( std::string &in_out )
            {
                in_out.assign( prepare_for_write( in_out.c_str( ),
                                                  in_out.size( ) ) );
            }

            virtual std::string prepare_for_write(const char *data,
                                                  size_t len) = 0;

            message_holder_sptr make_holder( const char *data, size_t length,
                                 vtrc::shared_ptr<system_closure_type> closure,
                                 bool on_send)
            {
                /// TODO: FIX IT
                message_holder_sptr mh(vtrc::make_shared<message_holder>());
                mh->message_ = std::string( data, data + length );
                mh->closure_ = closure;
                mh->on_send_ = on_send;
                return mh;
            }

            message_holder_sptr make_holder( const char *data, size_t length)
            {
                return make_holder(data, length,
                           vtrc::shared_ptr<system_closure_type>( ), false );
            }

            void write( const char *data, size_t length )
            {
                message_holder_sptr mh(make_holder(data, length));

#ifndef TRANSPORT_USE_ASYNC_WRITE

                vtrc::unique_lock<vtrc::mutex> lck(write_lock_);
                prepare_for_write( data->message_ );
                basio::write( *stream_, basio::buffer( mh->message_ ) );
#else
                write_dispatcher_.post(
                       vtrc::bind( &this_type::write_impl, this, mh,
                                    vtrc::shared_ptr<system_closure_type>( ),
                                    parent_->shared_from_this( )));
#endif
            }

            void write(const char *data, size_t length,
                       const system_closure_type &success, bool on_send)
            {

#ifndef TRANSPORT_USE_ASYNC_WRITE
                message_holder_sptr mh(make_holder(data, length));
                vtrc::unique_lock<vtrc::mutex> lck(write_lock_);
                prepare_for_write( data->message_ );
                bsys::error_code ec;
                basio::write( *stream_, basio::buffer( mh->message_ ), ec );
                success( ec );
#else
                vtrc::shared_ptr<system_closure_type>
                       closure(vtrc::make_shared<system_closure_type>(success));

                message_holder_sptr mh(make_holder(data, length,
                                                   closure, on_send));

                write_dispatcher_.post(
                       vtrc::bind( &this_type::write_impl, this, mh,
                                    closure, parent_->shared_from_this( )));
#endif
            }

#ifdef TRANSPORT_USE_ASYNC_WRITE

            void async_write(  )
            {
                async_write( write_queue_.front( )->message_.c_str( ),
                             write_queue_.front( )->message_.size( ), 0);
            }

            /// non concurrence call.
            void write_impl( message_holder_sptr data,
                             vtrc::shared_ptr<system_closure_type> closure,
                             common::connection_iface_sptr /*inst*/)
            {
                bool empty = write_queue_.empty( );

                prepare_for_write( data->message_ );

                write_queue_.push_back( data );
                if( closure && !data->on_send_ ) {
                    const bsys::error_code err(0, bsys::get_system_category( ));
                    (*closure)(err);
                }

                if( empty ) {
                    async_write( );
                }
            }

            /// because of _VARIADIC_MAX=5 for MSVC
            struct handler_params {
                size_t length_;
                size_t total_;
                common::connection_iface_sptr inst_;
                handler_params( size_t length,
                                size_t total,
                                common::connection_iface_sptr inst )
                    :length_(length)
                    ,total_(total)
                    ,inst_(inst)
                { }
            };

            void async_write( const char *data, size_t length, size_t total )
            {
                try {
                    stream_->async_write_some(
                            basio::buffer( data, length ),
                            write_dispatcher_.wrap(
                                    vtrc::bind( &this_type::write_handler, this,
                                        vtrc::placeholders::error,
                                        vtrc::placeholders::bytes_transferred,
                                        length, total, parent_->shared_from_this( )
//                                        handler_params(
//                                            length,
//                                            total,
//                                            parent_->shared_from_this( )
//                                        )
                                    )
                            ));
                } catch( const std::exception & ) {
                    this->close( );
                }

            }

            void write_handler( const bsys::error_code &error,
                                size_t const bytes,
                                size_t length_, size_t total_,
                                common::connection_iface_sptr inst )
                                //handler_params &params )
            {
                message_holder &top_holder(*write_queue_.front( ));

                if( !error ) {

                    if( bytes < length_ ) {

                        total_ += bytes;

                        const std::string &top(top_holder.message_);
                        async_write(top.c_str( ) + total_,
                                    top.size( )  - total_, total_);

                    } else {


                        if( top_holder.closure_ && top_holder.on_send_) {
                            (*top_holder.closure_)( error );
                        }

                        write_queue_.pop_front( );

                        if( !write_queue_.empty( ) )
                            async_write(  );
                    }
                } else {

                    if( top_holder.closure_ && top_holder.on_send_ ) {
                        (*top_holder.closure_)( error );
                    }

                    parent_->on_write_error( error );
                }
            }
#endif
            stream_type &get_socket( )
            {
                return *stream_;
            }

            const stream_type &get_socket( ) const
            {
                return *stream_;
            }

            boost::asio::io_service::strand &get_dispatcher( )
            {
                return write_dispatcher_;
            }
        };
    }

}}

#endif // VTRCTRANSPORTSTREAMIMPL_H
