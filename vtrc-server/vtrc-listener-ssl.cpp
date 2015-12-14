#include "vtrc-general-config.h"

#if 0 // VTRC_OPENSSL_ENABLED

#include "vtrc-common/vtrc-transport-ssl.h"
#include "vtrc-common/vtrc-protocol-accessor-iface.h"

#include "vtrc-listener-impl.h"
#include "vtrc-connection-impl.h"

namespace vtrc { namespace server { namespace listeners {

    namespace {

        namespace basio = boost::asio;
        namespace bssl  = boost::asio::ssl;
        namespace bip   = basio::ip;

        typedef bip::tcp::acceptor  acceptor_type;
        typedef bip::tcp::endpoint  endpoint_type;

        typedef common::transport_ssl            connection_type;
        typedef connection_impl<connection_type> connection_impl_type;


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

            bool is_local( ) const
            {
                return false;
            }

            static vtrc::shared_ptr<this_type> create( listener &listnr,
                                     vtrc::shared_ptr<socket_type> sock,
                                     const close_closure &on_close_cb,
                                     const std::string &name )
            {
                vtrc::shared_ptr<this_type> new_inst
                        (vtrc::make_shared<this_type>(vtrc::ref(listnr),
                                            sock,
                                            vtrc::ref(on_close_cb),
                                            vtrc::ref(name)));

                rpc::session_options const &opts(listnr.get_options( ));

                new_inst->protocol_.reset(
                        new protocol_layer_s( new_inst->app_,
                                                new_inst.get( ), opts ) );

                //new_inst->protocol_ ->init( );
                //new_inst->init( );
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

            void start( )
            {

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

            static endpoint_type make_endpoint( const std::string &address,
                                                unsigned short port )
            {
                return endpoint_type(bip::address::from_string(address), port);
            }

            std::string name( ) const
            {
                std::ostringstream oss;

                oss << "ssl://" << endpoint_.address( ).to_string( )
                    << ":"
                    << endpoint_.port( );

                return oss.str( );
            }

//            void connection_setup( vtrc::shared_ptr<connection_type> &con )
//            {
//                con->set_no_delay( no_delay_ );
//                std::ostringstream oss;
//                oss << "ssl://" << con->get_socket( )
//                                        .lowest_layer( ).remote_endpoint( );
//                con->set_name( oss.str( ) );
//            }

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
