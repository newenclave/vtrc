#ifndef VTRC_CONNECTION_IMPL_H
#define VTRC_CONNECTION_IMPL_H

namespace vtrc { namespace server { namespace endpoints {

    namespace {

        namespace basio = boost::asio;
        namespace bsys  = boost::system;

        template <typename ParentType>
        struct connection_impl: public ParentType {

            typedef ParentType                          parent_type;
            typedef connection_impl<ParentType>         this_type;
            typedef typename ParentType::socket_type    socket_type;

            endpoint_iface                     &endpoint_;
            application                        &app_;
            basio::io_service                  &ios_;
            common::enviroment                  env_;

            std::vector<char>                   read_buff_;

            vtrc::shared_ptr<protocol_layer_s>  protocol_;

            connection_impl(endpoint_iface &endpoint,
                            socket_type *sock, size_t buffer_size)
                :parent_type(sock)
                ,endpoint_(endpoint)
                ,app_(endpoint_.get_application( ))
                ,ios_(app_.get_io_service( ))
                ,env_(endpoint_.get_enviroment( ))
                ,read_buff_(buffer_size)
            {
                protocol_ = vtrc::make_shared<server::protocol_layer_s>
                                                     (vtrc::ref(app_), this);
            }

            static vtrc::shared_ptr<this_type> create(endpoint_iface &endpoint,
                            socket_type *sock,  size_t buffer_size )
            {
                vtrc::shared_ptr<this_type> new_inst
                            (vtrc::make_shared<this_type>
                                     (vtrc::ref(endpoint), sock, buffer_size));
                new_inst->init( );
                return new_inst;
            }

            void init( )
            {
                start_reading( );
                protocol_ ->init( );
            }

            bool ready( ) const
            {
                protocol_->ready( );
            }

            endpoint_iface &endpoint( )
            {
                return endpoint_;
            }

            std::string prepare_for_write( const char *data, size_t length )
            {
                return protocol_->prepare_data( data, length );
            }

            void on_write_error( const bsys::error_code &error )
            {
                protocol_->on_write_error( error );
                this->close( );
                //app_.get_clients( )->drop(this); // delete
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
                common::connection_iface_sptr lk(parent.lock( ));
                if( !lk ) return;

                if( !error ) {
                    try {
                        protocol_->process_data( &read_buff_[0], bytes );
                    } catch( const std::exception & /*ex*/ ) {
                        this->close( );
                        app_.get_clients( )->drop(this); // delete
                        return;
                    }
                    start_reading( );
                } else {
                    protocol_->on_read_error( error );
                    this->close( );
                    app_.get_clients( )->drop(this); // delete
                }
            }

            protocol_layer_s &get_protocol( )
            {
                return *protocol_;
            }
        };

    } // namespace

}}}

#endif // VTRCCONNECTIONIMPL_H
