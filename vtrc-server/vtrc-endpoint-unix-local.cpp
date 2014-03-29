
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

        typedef endpoint_impl<
            acceptor_type,
            endpoint_type,
            connection_type
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

        endpoint_iface *create(application &app,
                               const endpoint_options &opts,
                               const std::string &name)
        {
            ::unlink( name.c_str( ) );
            return new endpoint_unix( app, opts, name );
        }

        endpoint_iface *create( application &app, const std::string &name )
        {
            const endpoint_options def_opts = { 5, 20, 4096 };
            return create( app, def_opts, name );
        }

    }

}}}


#endif


