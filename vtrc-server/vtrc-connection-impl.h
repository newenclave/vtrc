#ifndef VTRC_CONNECTION_IMPL_H
#define VTRC_CONNECTION_IMPL_H

#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

#include "vtrc-listener.h"
#include "vtrc-application.h"
#include "vtrc-protocol-layer-s.h"
#include "vtrc-common/vtrc-enviroment.h"

#include <memory>

namespace vtrc { namespace server { namespace listeners {

    namespace {

        namespace basio = boost::asio;
        namespace bsys  = boost::system;

        typedef vtrc::function<
            void (const common::connection_iface *)
        > close_closure;

        template <typename SuperType>
        struct connection_impl: public SuperType {

            typedef SuperType                        super_type;
            typedef connection_impl<super_type>      this_type;
            typedef typename super_type::socket_type socket_type;

            listener                     &endpoint_;
            application                  &app_;
            basio::io_service            &ios_;
            common::enviroment            env_;

            std::vector<char>                   read_buff_;

            vtrc::unique_ptr<protocol_layer_s>  protocol_;
            close_closure                       close_closure_;

            connection_impl(listener &endpoint,
                            vtrc::shared_ptr<socket_type> sock,
                            const close_closure &on_close_cb)
                :super_type(sock)
                ,endpoint_(endpoint)
                ,app_(endpoint_.get_application( ))
                ,ios_(app_.get_io_service( ))
                ,env_(endpoint_.get_enviroment( ))
                ,read_buff_(endpoint_.get_options( ).read_buffer_size)
                ,close_closure_(on_close_cb)
            {
                protocol_.reset(new protocol_layer_s( vtrc::ref(app_), this,
                            endpoint_.get_options( ).maximum_active_calls,
                            endpoint_.get_options( ).maximum_message_length));
            }

            const std::string &id( ) const
            {
                return protocol_->client_id( );
            }

            static vtrc::shared_ptr<this_type> create(listener &endpoint,
                                   vtrc::shared_ptr<socket_type> sock,
                                   const close_closure &on_close_cb)
            {
                vtrc::shared_ptr<this_type> new_inst
                     (vtrc::make_shared<this_type>(vtrc::ref(endpoint),
                                                   sock, on_close_cb ));

                new_inst->init( );
                return new_inst;
            }

            static vtrc::shared_ptr<this_type> create(listener &endpoint,
                                  socket_type   *sock,
                                  close_closure &on_close_cb)
            {
                return create( endpoint,
                               vtrc::shared_ptr<socket_type>(sock),
                               on_close_cb );
            }

            void init( )
            {
                start_reading( );
                protocol_ ->init( );
            }

            common::enviroment &get_enviroment( )
            {
                return env_;
            }

            protocol_layer_s &get_protocol( )
            {
                return *protocol_;
            }

            const protocol_layer_s &get_protocol( ) const
            {
                return *protocol_;
            }

            bool ready( ) const
            {
                protocol_->ready( );
            }

            void close(  )
            {
                super_type::close( );
                protocol_->erase_all_slots( );
            }

            void on_close(  )
            {
                close_closure_( this );
            }

            bool active( ) const
            {
                return protocol_->ready( );
            }

            const common::call_context *get_call_context( ) const
            {
                return protocol_->get_call_context( );
            }

            std::string prepare_for_write( const char *data, size_t length )
            {
                return protocol_->prepare_data( data, length );
            }

            void on_write_error( const bsys::error_code &error )
            {
                protocol_->on_write_error( error );
                this->close( );
            }

            void close_drop( )
            {
                this->close( );
                app_.get_clients( )->drop(this); // delete
            }

            void start_reading( )
            {
#if 0
                basio::io_service::strand &disp(get_dispatcher( ));
                this->get_socket( ).async_read_some(
                        basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                        disp.wrap(vtrc::bind( &this_type::read_handler, this,
                             basio::placeholders::error,
                             basio::placeholders::bytes_transferred,
                             this->weak_from_this( )))
                    );
#else
                this->get_socket( ).async_read_some(
                        basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                            vtrc::bind( &this_type::read_handler, this,
                                 basio::placeholders::error,
                                 basio::placeholders::bytes_transferred,
                                 this->weak_from_this( ))
                    );
#endif
            }

            void read_handler( const bsys::error_code &error, size_t bytes,
                               common::connection_iface_wptr parent)
            {
                common::connection_iface_sptr lck(parent.lock( ));
                if( !lck ) return;

                if( !error ) {
                    try {
                        protocol_->process_data( &read_buff_[0], bytes );
                    } catch( const std::exception & /*ex*/ ) {
                        close_drop( );
                        return;
                    }
                    start_reading( );
                } else {
                    protocol_->on_read_error( error );
                    close_drop( );
                }
            }

        };

    } // namespace

}}}

#endif // VTRCCONNECTIONIMPL_H
