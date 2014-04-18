
#ifndef  _WIN32

#include "boost/asio/local/stream_protocol.hpp"

#include "vtrc-common/vtrc-transport-unix-local.h"

#include "vtrc-listener-impl.h"
#include "vtrc-connection-impl.h"

namespace vtrc { namespace server { namespace listeners {

    namespace {

        namespace basio   = boost::asio;
        typedef basio::local::stream_protocol bstream;

        typedef common::transport_unix_local    transport_type;
        typedef connection_impl<transport_type> connection_type;
        typedef bstream::endpoint       endpoint_type;
        typedef bstream::acceptor       acceptor_type;

#if 0
        struct unix_local_connenction_impl: public connection_type {

            typedef unix_local_connenction_impl this_type;

            unix_local_connenction_impl( endpoint_iface &endpoint,
                                    vtrc::shared_ptr<socket_type> sock )
                :connection_type(endpoint, sock)
            { }


            bool impersonate( )
            {
                ucred cred = { 0 };
                unsigned len = sizeof(cred);
                if (getsockopt(get_socket( ).native_handle( ),
                               SOL_SOCKET, SO_PEERCRED, &cred, &len) == -1)
                {
                } else {
                }
                return false;
            }

            void revert( )
            {

            }

            static vtrc::shared_ptr<this_type> create(endpoint_iface &endpoint,
                                                        socket_type *sock )
            {
                vtrc::shared_ptr<this_type> new_inst
                     (vtrc::make_shared<this_type>(vtrc::ref(endpoint),
                                        vtrc::shared_ptr<socket_type>(sock) ));

                new_inst->init( );
                return new_inst;
            }
        };

        typedef endpoint_impl<
            acceptor_type,
            endpoint_type,
            unix_local_connenction_impl
        > super_type;

#else
        typedef listener_impl<
            acceptor_type,
            endpoint_type,
            connection_type
        > super_type;
#endif

        struct listener_unix: public super_type {

            listener_unix( application &app,
                           const listener_options &opts,
                           const std::string &name )
                :super_type(app, opts, endpoint_type(name))
            { }

            std::string string( ) const
            {
                std::ostringstream oss;
                oss << "unix://" << endpoint_.path( );
                return oss.str( );
            }
        };
    }

    namespace unix_local {

        listener_sptr create(application &app,
                               const listener_options &opts,
                               const std::string &name)
        {
            vtrc::shared_ptr<listener_unix>new_l
                    (vtrc::make_shared<listener_unix>(
                         vtrc::ref(app), vtrc::ref(opts),
                         vtrc::ref(name) ) );

            app.attach_listener( new_l );
            return new_l;
        }

        listener_sptr create( application &app, const std::string &name )
        {
            const listener_options def_opts(default_options( ));
            return create( app, def_opts, name );
        }

    }

}}}


#endif

