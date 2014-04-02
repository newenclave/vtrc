
#ifndef  _WIN32

#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>

#include "vtrc-common/vtrc-transport-unix-local.h"

#include "vtrc-endpoint-impl.h"
#include "vtrc-connection-impl.h"

namespace vtrc { namespace server { namespace endpoints {

    namespace {

        namespace basio   = boost::asio;
        typedef basio::local::stream_protocol bstream;

        typedef common::transport_unix_local    transport_type;
        typedef connection_impl<transport_type> connection_type;
        typedef bstream::endpoint       endpoint_type;
        typedef bstream::acceptor       acceptor_type;

#if 0
        struct local_connenction_impl: public connection_type {

            typedef local_connenction_impl this_type;

            local_connenction_impl( endpoint_iface &endpoint,
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
#endif
        typedef endpoint_impl<
            acceptor_type,
            endpoint_type,
            connection_type
//            local_connenction_impl
        > super_type;

        struct endpoint_unix: public super_type {

            endpoint_unix( application &app,
                           const endpoint_options &opts,
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

        endpoint_options default_options( )
        {
            endpoint_options def_opts = { 5, 1024 * 1024, 20, 4096 };
            return def_opts;
        }

        endpoint_iface *create(application &app,
                               const endpoint_options &opts,
                               const std::string &name)
        {
            return new endpoint_unix( app, opts, name );
        }

        endpoint_iface *create( application &app, const std::string &name )
        {
            const endpoint_options def_opts(default_options( ));
            return create( app, def_opts, name );
        }

    }

}}}


#endif


