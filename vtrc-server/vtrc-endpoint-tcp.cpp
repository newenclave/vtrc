
#include <boost/asio.hpp>

#include "vtrc-common/vtrc-transport-tcp.h"

#include "vtrc-endpoint-impl.h"
#include "vtrc-connection-impl.h"

namespace vtrc { namespace server { namespace endpoints {

    namespace {

        namespace basio = boost::asio;
        namespace bip   = basio::ip;

        typedef common::transport_tcp           transport_type;
        typedef connection_impl<transport_type> connection_type;
        typedef bip::tcp::endpoint endpoint_type;

        typedef endpoint_impl <
                bip::tcp::acceptor,
                bip::tcp::endpoint,
                connection_type
        > super_type;

        struct endpoint_tcp: public super_type {

            static endpoint_type make_endpoint( const std::string &address,
                                                unsigned short port )
            {
                return endpoint_type(bip::address::from_string(address), port);
            }

            endpoint_tcp( application &app,
                          const endpoint_options &opts,
                          const std::string &address,
                          unsigned short port )
                :super_type( app, opts, make_endpoint(address, port))
            {}

            virtual std::string string( ) const
            {
                std::ostringstream oss;
                oss << "tcp://" << endpoint_.address( ).to_string( )
                    << ":"
                    << endpoint_.port( );
                return oss.str( );
            }

        };
    }
    namespace tcp {

        endpoint_iface *create(application &app, const endpoint_options &opts,
                            const std::string &address, unsigned short service)
        {
            return new endpoint_tcp( app, opts, address, service );
        }

        endpoint_iface *create( application &app,
                                const std::string &address,
                                unsigned short service )
        {
            const endpoint_options def_opts = { 5, 20, 4096 };
            return create( app, def_opts, address, service );
        }
    }

}}}
