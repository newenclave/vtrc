#include "vtrc-general-config.h"

#if 0 // VTRC_OPENSSL_ENABLED

#include "vtrc-common/vtrc-transport-ssl.h"
#include "vtrc-common/vtrc-protocol-accessor-iface.h"

#include "vtrc-listener-impl.h"
#include "vtrc-connection-impl.h"

#include "vtrc-memory.h"

//
//
//

namespace vtrc { namespace server { namespace listeners {

    namespace {

        namespace basio = boost::asio;
        namespace bssl  = boost::asio::ssl;
        namespace bip   = basio::ip;

        typedef bip::tcp::acceptor  acceptor_type;
        typedef bip::tcp::endpoint  endpoint_type;

        typedef common::transport_ssl                  connection_type;
        typedef connection_impl<connection_type>       connection_impl_type;
        typedef typename connection_type::socket_type  socket_type;

        typedef vtrc::function<
            void (const common::connection_iface *)
        > close_closure;

        struct commection_impl: public connection_impl_type {

            typedef commection_impl this_type;
            std::string name_;

            commection_impl( listener &listen,
                                  vtrc::shared_ptr<socket_type> sock,
                                  const close_closure &on_close_cb,
                                  const std::string &name )
                :connection_impl_type(listen, sock, on_close_cb)
                ,name_(name)
            { }

            std::string name(  ) const
            {
                return name_;
            }

            void set_name( const std::string &name )
            {
                name_ = name;
            }

            bool is_local( ) const
            {
                return false;
            }

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

            static vtrc::shared_ptr<this_type> create( listener &listnr,
                                     vtrc::shared_ptr<socket_type> sock,
                                     const close_closure &on_close_cb )
            {
                vtrc::shared_ptr<this_type> new_inst
                        (vtrc::make_shared<this_type>(vtrc::ref(listnr),
                                            sock,
                                            vtrc::ref(on_close_cb),
                                            "" ) );

                rpc::session_options const &opts(listnr.get_options( ));

                new_inst->protocol_.reset(
                        new protocol_layer_s( new_inst->app_,
                                                new_inst.get( ), opts ) );

                return new_inst;
            }
        };

        endpoint_type make_endpoint( const std::string &address,
                                     unsigned short port )
        {
            return endpoint_type(bip::address::from_string(address), port);
        }


        struct listener_ssl: public listener {

            typedef listener_ssl this_type;

            basio::io_service              &ios_;
            endpoint_type                   endpoint_;
            vtrc::unique_ptr<acceptor_type> acceptor_;
            bool                            working_;
            bool                            no_delay_;
            basio::ssl::context             context_;

            listener_ssl( application &app,
                          const rpc::session_options &opts,
                          const std::string &address,
                          unsigned short port, bool no_delay )
                :listener(app, opts)
                ,ios_(app.get_io_service( ))
                ,endpoint_(make_endpoint(address, port))
                ,working_(false)
                ,no_delay_(no_delay)
                ,context_(bssl::context::sslv23)
            { }

            void on_client_destroy( vtrc::weak_ptr<listener> inst,
                                    const common::connection_iface *conn )
            {
                vtrc::shared_ptr<listener> lock( inst.lock( ) );
                if( lock ) {
                    this->stop_connection( conn );
                }
            }

            close_closure get_on_close_cb( )
            {
                return vtrc::bind( &this_type::on_client_destroy, this,
                                   weak_from_this( ), vtrc::placeholders::_1 );
            }

            void start( )
            {
                context_.set_options(
                    bssl::context::default_workarounds
                    | bssl::context::no_sslv2
                    | bssl::context::single_dh_use);
//                context_.set_password_callback(
//                            vtrc::bind( &server::get_password, this ) );
                context_.use_certificate_chain_file("server.crt");
                context_.use_private_key_file("server.key", bssl::context::pem);
                context_.use_tmp_dh_file("dh1024.pem");

                working_ = true;
                acceptor_.reset(new acceptor_type(ios_, endpoint_));
                acceptor_->listen( 5 );
                start_accept( );
                call_on_start( );
            }

            void start_accept( )
            {
                vtrc::shared_ptr<socket_type> new_sock
                                            ( new socket_type(ios_, context_) );

//                    ( vtrc::make_shared<socket_type>( vtrc::ref(ios_),
//                                                      vtrc::ref(context_)));

                acceptor_->async_accept( new_sock->lowest_layer( ),
                    vtrc::bind( &this_type::on_accept, this,
                                 vtrc::placeholders::error, new_sock,
                                 weak_from_this( )));
            }

            void stop ( )
            {

            }

            bool is_active( ) const
            {
                return working_;
            }

            bool is_local( ) const
            {
                return false;
            }

            void on_accept( const bsys::error_code &error,
                            vtrc::shared_ptr<socket_type> sock,
                            vtrc::weak_ptr<listener> &inst )
            {
                vtrc::shared_ptr<listener> locked( inst.lock( ));
                if( !locked ) {
                    return;
                }

                if( !error ) {
                    vtrc::shared_ptr<commection_impl> new_conn;
                    try {
                        new_conn = commection_impl::create( *this, sock,
                                                            get_on_close_cb( ));

                        connection_setup( new_conn );

                        namespace ph = vtrc::placeholders;

                        new_conn->get_protocol( )
                                 .set_precall( vtrc::bind(
                                                  &this_type::mk_precall, this,
                                                   ph::_1, ph::_2, ph::_3 ) );
                        new_conn->get_protocol( )
                                 .set_postcall( vtrc::bind(
                                                  &this_type::mk_postcall, this,
                                                   ph::_1, ph::_2 ) );

                        get_application( ).get_clients( )->store( new_conn );
                        new_connection( new_conn.get( ) );

                        new_conn->init( );

    //                    get_application( ).get_io_service( )
    //                        .dispatch( vtrc::bind( &connection_type::init,
    //                                                new_conn ) );

                    } catch( ... ) {
                        //sock->close( );
                        if( new_conn.get( ) )  {
                            get_application( ).get_clients( )
                                             ->drop( new_conn.get( ) );
                        }
                    }
                    start_accept( );
                } else {
                    if( working_ ) {
                        stop( );
                        call_on_accept_failed( error );
                    } else {
                        call_on_stop( );
                    }
                }
            }

            std::string name( ) const
            {
                std::ostringstream oss;

                oss << "ssl://" << endpoint_.address( ).to_string( )
                    << ":"
                    << endpoint_.port( );

                return oss.str( );
            }

            void connection_setup( vtrc::shared_ptr<commection_impl> &con )
            {
                con->set_no_delay( no_delay_ );
                std::ostringstream oss;
                oss << "ssl://" << con->get_socket( )
                                        .lowest_layer( ).remote_endpoint( );
                con->set_name( oss.str( ) );
            }

        };
    }

    namespace tcp_ssl {

        listener_sptr create(application &app,
                             const rpc::session_options &opts,
                             const std::string &address, unsigned short service,
                             bool tcp_nodelay)
        {

            vtrc::shared_ptr<listener_ssl>new_l
                    (vtrc::make_shared<listener_ssl>(
                             vtrc::ref(app), vtrc::ref(opts),
                             vtrc::ref(address), service,
                             tcp_nodelay ));
            return new_l;
        }

        listener_sptr create( application &app,
                              const std::string &address,
                              unsigned short service, bool tcp_nodelay )
        {
            const rpc::session_options
                    def_opts( common::defaults::session_options( ) );

            return create( app, def_opts, address, service, tcp_nodelay );
        }

    }

}}}

#endif
