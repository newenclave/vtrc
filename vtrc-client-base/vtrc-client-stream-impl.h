#ifndef VTRC_CLIENT_STREAM_IMPL_H
#define VTRC_CLIENT_STREAM_IMPL_H

#include "boost/asio.hpp"

#include "vtrc-client.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"
#include "vtrc-chrono.h"

#include "vtrc-protocol-layer-c.h"

namespace vtrc { namespace client {

namespace {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;

    template <typename ParentType, typename StreamType>
    struct client_stream_impl {

        typedef ParentType                                    parent_type;
        typedef StreamType                                    stream_type;
        typedef client_stream_impl<parent_type, stream_type>  this_type;

        boost::asio::io_service &ios_;

        parent_type             *parent_;
        vtrc_client             *client_;

        std::vector<char>        read_buff_;

        vtrc::unique_ptr<protocol_layer_c> protocol_;

        client_stream_impl( boost::asio::io_service &ios,
                            vtrc_client *client, size_t read_buffer_size )
            :ios_(ios)
            ,client_(client)
            ,read_buff_(read_buffer_size)
        {

        }

        virtual ~client_stream_impl( ) { }

        void set_parent( parent_type *parent )
        {
            parent_ = parent;
        }

        const std::string &id( ) const
        {
            return protocol_->client_id( );
        }

        stream_type &get_socket( )
        {
            return parent_->get_socket( );
        }

        void init(  )
        {
            protocol_.reset(new protocol_layer_c( parent_, client_ ));
        }

        void on_close( )
        {
            protocol_->close( );
        }

        bool active( ) const
        {
            return protocol_->ready( );
        }

        void on_connect( const boost::system::error_code &err,
                         common::closure_type closure,
                         common::connection_iface_sptr /*parent*/)
        {
            if( !err ) {
                start_reading( );
            }
            closure( err );
        }

        void start_reading( )
        {
#if 0
            basio::io_service::strand &disp(parent_->get_dispatcher( ));

            sock( ).async_read_some(
                    basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                    disp.wrap(vtrc::bind( &this_type::read_handler, this,
                         basio::placeholders::error,
                         basio::placeholders::bytes_transferred,
                         parent_->weak_from_this( ) ))
                );
#else
            get_socket( ).async_read_some(
                    basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                        vtrc::bind( &this_type::read_handler, this,
                             basio::placeholders::error,
                             basio::placeholders::bytes_transferred,
                             parent_->weak_from_this( ) )
                );
#endif
        }

        void read_handler( const bsys::error_code &error, size_t bytes,
                           common::connection_iface_wptr parent)
        {
            common::connection_iface_sptr lk(parent.lock( ));
            if( !lk ) return;

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

        const common::call_context *get_call_context( ) const
        {
            return protocol_->get_call_context( );
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

}
}}

#endif // VTRC_CLIENT_STREAM_IMPL_H
