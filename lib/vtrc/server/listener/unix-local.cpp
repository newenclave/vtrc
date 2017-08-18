
#ifndef  _WIN32

#include "vtrc-asio.h"

#include "vtrc/server/listener/impl.h"
#include "vtrc/server/connection/impl.h"

#include "vtrc/common/transport/unix_local.h"
#include "vtrc/common/protocol/accessor-iface.h"

#include <errno.h>

namespace vtrc { namespace server { namespace listeners {

    namespace {

        namespace basio = VTRC_ASIO;
        typedef basio::local::stream_protocol bstream;

        typedef common::transport_unix_local    transport_type;
        typedef connection_impl<transport_type> connection_type;
        typedef bstream::endpoint               endpoint_type;
        typedef bstream::acceptor               acceptor_type;

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
                           const rpc::session_options &opts,
                           const std::string &name )
                :super_type(app, opts, endpoint_type(name))
            { }

            void stop_impl( )
            {
                ::unlink( endpoint_.path( ).c_str( ) );
            }

            std::string name( ) const
            {
                std::ostringstream oss;
                oss << "unix://" << endpoint_.path( );
                return oss.str( );
            }

            bool is_local( ) const
            {
                return true;
            }

            void connection_setup( vtrc::shared_ptr<connection_type> &con )
            {
                std::ostringstream oss;
                oss << "unix://" << endpoint_.path( );

#ifdef SO_PEERCRED
                ucred cred = { 0 };
                unsigned len = sizeof(cred);

                int opt_res = ::getsockopt( con->get_socket( ).native_handle( ),
                                            SOL_SOCKET, SO_PEERCRED, &cred,
                                            reinterpret_cast<socklen_t*>(&len));

                if ( opt_res != -1 ) {
                    oss << "["
                        << "p:" << cred.pid << ", "
                        << "g:" << cred.gid << ", "
                        << "u:" << cred.uid
                        << "]"
                        ;
                } else {
                    oss << "[get PEERCRED error " << errno << "]";
                }
#endif
                con->set_name( oss.str( ) );
            }

        };
    }

    namespace unix_local {

        listener_sptr create(application &app,
                               const rpc::session_options &opts,
                               const std::string &name)
        {
            vtrc::shared_ptr<listener_unix>new_l
                    (vtrc::make_shared<listener_unix>(
                         vtrc::ref(app), vtrc::ref(opts),
                         vtrc::ref(name) ) );
            return new_l;
        }

        listener_sptr create( application &app, const std::string &name )
        {
            const rpc::session_options
                    def_opts( common::defaults::session_options( ) );
            return create( app, def_opts, name );
        }

    }

}}}


#endif


