#ifndef VTRC_CONNECTION_IMPL_H
#define VTRC_CONNECTION_IMPL_H

#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"

#include "vtrc-listener.h"
#include "vtrc-application.h"
#include "vtrc-protocol-layer-s.h"

#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-common/vtrc-exception.h"

#include "vtrc-rpc-lowlevel.pb.h"

#include <memory>

#include "vtrc-connection-closure.h"

namespace vtrc { namespace server { namespace listeners {

    namespace {

        namespace basio = VTRC_ASIO;
        namespace bsys  = VTRC_SYSTEM;

        typedef vtrc::server::connection_close_closure close_closure;

        template <typename SuperType>
        struct connection_impl: public SuperType {

            typedef SuperType                        super_type;
            typedef connection_impl<super_type>      this_type;
            typedef typename super_type::socket_type socket_type;

            listener                           &endpoint_;
            application                        &app_;
            basio::io_service                  &ios_;
            common::enviroment                  env_;

            std::vector<char>                   read_buff_;

            vtrc::unique_ptr<protocol_layer_s>  protocol_;
            const close_closure                 close_closure_;

            std::string                         name_;

            connection_impl(listener &endpoint,
                            vtrc::shared_ptr<socket_type> sock,
                            const close_closure &on_close_cb)
                :super_type(sock)
                ,endpoint_(endpoint)
                ,app_(endpoint_.get_application( ))
                ,ios_(app_.get_io_service( ))
                ,env_(endpoint_.get_enviroment( ))
                ,read_buff_(endpoint_.get_options( ).read_buffer_size( ))
                ,close_closure_(on_close_cb)
            { }

            ~connection_impl( )
            { }

            void set_name( std::string const &name )
            {
                name_.assign( name );
            }

            const std::string &id( ) const
            {
                return protocol_->client_id( );
            }

            static vtrc::shared_ptr<this_type>
                create(listener &endpoint, vtrc::shared_ptr<socket_type> sock,
                                           const close_closure &on_close_cb)
            {
                vtrc::shared_ptr<this_type> new_inst
                     (vtrc::make_shared<this_type>(vtrc::ref(endpoint),
                                                   sock, on_close_cb ));

                rpc::session_options const &opts(endpoint.get_options( ));

                new_inst->protocol_.reset(
                       new protocol_layer_s( new_inst->app_,
                                             new_inst.get( ), opts ) );

                //new_inst->protocol_->init( );

                return new_inst;
            }

//            static vtrc::shared_ptr<this_type> create(
//                                    listener &endpoint, socket_type *sock,
//                                    close_closure &on_close_cb)
//            {
//                return create( endpoint,
//                               vtrc::shared_ptr<socket_type>(sock),
//                               on_close_cb );
//            }

            void start_read_clos( bsys::error_code const & /*err*/,
                                  common::connection_iface_wptr &inst)
            {
                common::connection_iface_sptr lck(inst.lock( ));
                if( lck ) {
                    start_reading( );
                }
            }

            void init( )
            {
                protocol_->init_success(
                            vtrc::bind( &this_type::start_read_clos, this,
                                        vtrc::placeholders::error,
                                        this->weak_from_this( )));
                //start_reading( );
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

            void close(  )
            {
                super_type::close( );
                protocol_->erase_all_slots( );
            }

            void on_close(  )
            {
                close_closure_( this );
            }

            std::string name( ) const
            {
                return name_;
            }

            bool active( ) const
            {
                return protocol_->ready( );
            }

            const common::call_context *get_call_context( ) const
            {
                return protocol_->get_call_context( );
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
                    vtrc::common::raise(
                                std::runtime_error( "Not connected." ) );
                    return;
                }
                protocol_->make_local_call( ll_mess,
                           vtrc::bind( &this_type::success_closure, this,
                                       vtrc::placeholders::_1, done ) );
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

#if 0
            void close_drop( const std::string &mess )
            {
                rpc::lowlevel_unit llu;
                typedef rpc::message_info message_info;

                llu.mutable_info( )
                        ->set_message_type( message_info::MESSAGE_SERVER_CALL );
                llu.mutable_error( )
                        ->set_category( rpc::errors::CATEGORY_INTERNAL);
                llu.mutable_error( )
                        ->set_code( rpc::errors::ERR_PROTOCOL );
                llu.mutable_error( )
                        ->set_additional( mess );

                protocol_->send_and_close( llu );

                app_.get_clients( )->drop(this); // delete
            }
#endif
            void start_reading( )
            {
#if 0
                basio::io_service::strand &disp(get_dispatcher( ));
                this->get_socket( ).async_read_some(
                        basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                        disp.wrap(vtrc::bind( &this_type::read_handler, this,
                             vtrc::placeholders::error,
                             vtrc::placeholders::bytes_transferred,
                             this->weak_from_this( )))
                    );
#else
                DEBUG_LINE(this);

                this->get_socket( ).async_read_some(
                        basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                            vtrc::bind( &this_type::read_handler, this,
                                 vtrc::placeholders::error,
                                 vtrc::placeholders::bytes_transferred,
                                 this->weak_from_this( ))
                    );
#endif
            }

            void read_handler( const bsys::error_code &error, size_t bytes,
                               const common::connection_iface_wptr &inst )
            {
                common::connection_iface_sptr lck(inst.lock( ));
                if( !lck ) {
                    return;
                }

                if( !error ) {
                    try {
                        protocol_->process_data( &read_buff_[0], bytes );
                    } catch( const std::exception & /*ex*/ ) {
                        close_drop(  );
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
