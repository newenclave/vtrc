#ifndef VTRC_CLIENT_STREAM_IMPL_H
#define VTRC_CLIENT_STREAM_IMPL_H

#include "vtrc-asio.h"

#include "vtrc-client.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"
#include "vtrc-chrono.h"

#include "vtrc-protocol-layer-c.h"
#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-common/vtrc-exception.h"

namespace vtrc { namespace client {

namespace { /// implementation.

    namespace basio = VTRC_ASIO;
    namespace bsys  = VTRC_SYSTEM;

    template <typename ParentType, typename StreamType>
    struct client_stream_impl {

        typedef ParentType                                    parent_type;
        typedef StreamType                                    stream_type;
        typedef client_stream_impl<parent_type, stream_type>  this_type;

        VTRC_ASIO::io_service &ios_;

        parent_type             *parent_;
        base                    *client_;
        protocol_signals        *callbacks_;
        common::enviroment       env_;

        std::vector<char>        read_buff_;

        vtrc::unique_ptr<protocol_layer_c> protocol_;

        client_stream_impl( VTRC_ASIO::io_service &ios,
                            base *client, protocol_signals *callbacks,
                            size_t read_buffer_size )
            :ios_(ios)
            ,parent_(NULL)
            ,client_(client)
            ,callbacks_(callbacks)
            ,read_buff_(read_buffer_size)
        {

        }

        virtual ~client_stream_impl( ) { }

        void set_parent( parent_type *parent )
        {
            parent_ = parent;
        }

        parent_type *get_parent( )
        {
            return parent_;
        }

        const std::string &id( ) const
        {
            return protocol_->client_id( );
        }

        stream_type &get_socket( )
        {
            return parent_->get_socket( );
        }

        common::enviroment &get_enviroment( )
        {
            return env_;
        }

        void init(  )
        {
            protocol_.reset(new protocol_layer_c( parent_, client_,
                                                  callbacks_ ));
            protocol_->init( );
        }

        void on_close( )
        {
            protocol_->close( );
        }

        bool active( ) const
        {
            return protocol_->ready( );
        }

        virtual void connection_setup(  )
        {
            ;;;
        }

        void on_connect( const bsys::error_code &err,
                         common::system_closure_type closure,
                         common::connection_iface_sptr /*parent*/)
        {
            if( !err ) {
                connection_setup( );
                start_reading( );
            }
            closure( err );
        }

        void start_reading( )
        {
#if 0
            basio::io_service::strand &disp(parent_->get_dispatcher( ));

            get_socket( ).async_read_some(
                    basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                    disp.wrap(vtrc::bind( &this_type::read_handler, this,
                         vtrc::placeholders::error,
                         vtrc::placeholders::bytes_transferred,
                         parent_->shared_from_this( ) ))
                );
#else
            DEBUG_LINE(parent_);

            get_socket( ).async_read_some(
                    basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                        vtrc::bind( &this_type::read_handler, this,
                             vtrc::placeholders::error,
                             vtrc::placeholders::bytes_transferred,
                             //parent_->shared_from_this( ),
                             client_->weak_from_this( ) )
                );
#endif
        }

        void read_handler( const bsys::error_code &error, size_t bytes,
//                           const common::connection_iface_sptr /*inst*/,
                           const base_wptr client )
        {
            client::base_sptr lck(client.lock( ));
            if( !lck ) {
                return;
            }
//            common::connection_iface_sptr lk(inst.lock( ));
//            if( !lk ) return;

            if( !error ) {
                try {
                    protocol_->process_data( &read_buff_[0], bytes );
                } catch( const std::exception & /*ex*/ ) {
                    //protocol_->on_read_error( error );
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

        client::protocol_layer_c &get_protocol( )
        {
            return *protocol_;
        }

        const client::protocol_layer_c &get_protocol( ) const
        {
            return *protocol_;
        }

        void on_write_error( const VTRC_SYSTEM::error_code &err )
        {
            protocol_->on_write_error( err );
        }

        void success_closure( const rpc::errors::container &cont,
                              common::empty_closure_type &done )
        {
            done( );
        }

        void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                              common::empty_closure_type done )
        {
            if( !protocol_.get( ) ) {
                common::raise( std::runtime_error( "Not connected." ) );
                return;
            }
            protocol_->make_local_call( ll_mess,
                       vtrc::bind( &this_type::success_closure, this,
                                   vtrc::placeholders::_1, done ) );
        }
    };

}  // namespace {

}} // namespace vtrc { namespace client {

#endif // VTRC_CLIENT_STREAM_IMPL_H
