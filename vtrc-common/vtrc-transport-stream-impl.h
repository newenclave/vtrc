#ifndef VTRC_TRANSPORT_STREAM_IMPL_H
#define VTRC_TRANSPORT_STREAM_IMPL_H


#include <deque>
#include <string>

#include "vtrc-asio.h"

#include "vtrc-enviroment.h"
#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-mutex.h"
#include "vtrc-closure.h"

#include "vtrc-chrono.h"

namespace vtrc { namespace common {

    namespace basio = VTRC_ASIO;
    namespace bsys  = VTRC_SYSTEM;

#define TRANSPORT_USE_SYNC_WRITE 0

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
                    :on_send_(false)
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

            std::deque<message_holder_sptr>      write_queue_;

            transport_impl( vtrc::shared_ptr<stream_type> s,
                            const std::string &n )
                :stream_(s)
                ,ios_(stream_->get_io_service( ))
                ,write_dispatcher_(ios_)
                ,parent_(NULL)
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
                    parent_->on_close( );
                    close_handle( );
                }
            }

            virtual void close_handle( ) = 0;

            bool closed( ) const
            {
                return closed_;
            }

            VTRC_ASIO::io_service &get_io_service( )
            {
                return ios_;
            }

            static
            message_holder_sptr make_holder( const char *data, size_t length,
                                 vtrc::shared_ptr<system_closure_type> closure,
                                 bool on_send )
            {
                /// TODO: FIX IT
                message_holder_sptr mh(vtrc::make_shared<message_holder>());
                mh->message_ = std::string( data, data + length );
                mh->closure_ = closure;
                mh->on_send_ = on_send;
                return mh;
            }

            static
            message_holder_sptr make_holder( const char *data, size_t length )
            {
                return make_holder( data, length,
                           vtrc::shared_ptr<system_closure_type>( ), false );
            }

            void write( const char *data, size_t length )
            {
                message_holder_sptr mh(make_holder(data, length));

                DEBUG_LINE(parent_);

                write_dispatcher_.post(
                       vtrc::bind( &this_type::write_impl, this, mh,
                                    vtrc::shared_ptr<system_closure_type>( ),
                                    parent_->shared_from_this( )));
            }

            void write( const char *data, size_t length,
                        const system_closure_type &success, bool on_send )
            {

                vtrc::shared_ptr<system_closure_type>
                       closure(vtrc::make_shared<system_closure_type>(success));

                message_holder_sptr mh( make_holder( data, length,
                                                     closure, on_send ) );
                DEBUG_LINE(parent_);

                write_dispatcher_.post(
                       vtrc::bind( &this_type::write_impl, this, mh,
                                    closure, parent_->shared_from_this( )));
            }

            void async_write(  )
            {
                async_write( write_queue_.front( )->message_.c_str( ),
                             write_queue_.front( )->message_.size( ), 0 );
            }

            /// non concurrence call.
            void write_impl( message_holder_sptr data,
                             vtrc::shared_ptr<system_closure_type> closure,
                             common::connection_iface_sptr &/*inst*/)
            {
//                common::connection_iface_sptr lckd( inst.lock( ) );
//                if( !lckd ) {
//                    return;
//                }

                bool empty = write_queue_.empty( );

                get_parent( )->get_protocol( )
                              .get_lowlevel( )
                             ->pack_message( data->message_ );

                write_queue_.push_back( data );

                if( closure && !data->on_send_ ) {
                    static const bsys::error_code err;
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
                common::connection_iface_wptr inst_;
                handler_params( size_t length, size_t total,
                                common::connection_iface_sptr inst )
                    :length_(length)
                    ,total_(total)
                    ,inst_(inst)
                { }
            };

            void async_write( const char *data, size_t length, size_t total )
            {
                DEBUG_LINE(parent_);

                //try { /// TODO
                if( !closed_ ) {
                    stream_->async_write_some(
                            basio::buffer( data, length ),
                            write_dispatcher_.wrap(
                                    vtrc::bind( &this_type::write_handler, this,
                                        vtrc::placeholders::error,
                                        vtrc::placeholders::bytes_transferred,
                                        handler_params(
                                            length,
                                            total,
                                            parent_->shared_from_this( )
                                        )
                                    )
                            ));
                }
//                } catch( const std::exception & ) {
//                    this->close( );
//                }

            }

            void write_handler( const bsys::error_code &error,
                                size_t const bytes,
                                handler_params &params )
            {

                common::connection_iface_sptr lock(params.inst_.lock( ));
                if( !lock ) {
                    return;
                }

                message_holder &top_holder(*write_queue_.front( ));

                if( !error ) {

                    if( bytes < params.length_ ) {

                        params.total_ += bytes;

                        const std::string &top(top_holder.message_);

                        async_write( top.c_str( ) + params.total_,
                                     top.size( )  - params.total_,
                                     params.total_ );

                    } else {

                        if( top_holder.closure_ && top_holder.on_send_) {
                            (*top_holder.closure_)( error );
                        }

                        write_queue_.pop_front( );

                        if( !write_queue_.empty( ) ) {
                            async_write(  );
                        }

                    }
                } else {

                    if( top_holder.closure_ && top_holder.on_send_ ) {
                        (*top_holder.closure_)( error );
                    }

                    parent_->on_write_error( error );
                }
            }

            stream_type &get_socket( )
            {
                return *stream_;
            }

            const stream_type &get_socket( ) const
            {
                return *stream_;
            }

            VTRC_ASIO::io_service::strand &get_dispatcher( )
            {
                return write_dispatcher_;
            }
        };
    }

}}

#endif // VTRCTRANSPORTSTREAMIMPL_H
