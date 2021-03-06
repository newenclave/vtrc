
#include "vtrc/common/transport/tcp.h"
#include "vtrc/common/protocol/accessor-iface.h"

#include "vtrc/server/listener/impl.h"
#include "vtrc/server/connection/impl.h"

namespace vtrc { namespace server { namespace listeners {

    namespace {

        namespace basio = VTRC_ASIO;
        namespace bip   = basio::ip;

        typedef common::transport_tcp           transport_type;
        typedef connection_impl<transport_type> connection_type;
        typedef bip::tcp::endpoint              endpoint_type;

        typedef listener_impl <
                bip::tcp::acceptor,
                bip::tcp::endpoint,
                connection_type
        > super_type;

        struct listener_tcp: public super_type {

            bool no_delay_;
            std::string name_;

            static endpoint_type make_endpoint( const std::string &address,
                                                unsigned short port )
            {
                return endpoint_type(bip::address::from_string(address), port);
            }

            listener_tcp( application &app,
                          const rpc::session_options &opts,
                          const std::string &address,
                          unsigned short port, bool no_delay )
                :super_type( app, opts, make_endpoint(address, port))
                ,no_delay_(no_delay)
            {
                std::ostringstream oss;
                oss << "tcp://" << endpoint_.address( ).to_string( )
                    << ":" << endpoint_.port( );
                name_ = oss.str( );
            }

            std::string name( ) const
            {
                return name_;
            }

            bool is_local( ) const
            {
                return false;
            }

            void start_success( )
            {
                std::ostringstream oss;
                oss << "tcp://" << endpoint_.address( ).to_string( )
                    << ":"
                    << acceptor( )->local_endpoint( ).port( );
                name_ = oss.str( );
            }

            void connection_setup( vtrc::shared_ptr<connection_type> &con )
            {
                con->set_no_delay( no_delay_ );
                std::ostringstream oss;
                oss << "tcp://" << con->get_socket( ).remote_endpoint( );
                con->set_name( oss.str( ) );
            }

        };
    }

    namespace tcp {

        listener_sptr create(application &app,
                             const rpc::session_options &opts,
                             const std::string &address, unsigned short service,
                             bool tcp_nodelay)
        {

            vtrc::shared_ptr<listener_tcp>new_l
                    (vtrc::make_shared<listener_tcp>(
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
