
#include "vtrc-common/vtrc-transport-tcp.h"

#include "vtrc-listener-impl.h"
#include "vtrc-connection-impl.h"

namespace vtrc { namespace server { namespace listeners {

    namespace {

        namespace basio = boost::asio;
        namespace bip   = basio::ip;

        typedef common::transport_tcp           transport_type;
        typedef connection_impl<transport_type> connection_type;
        typedef bip::tcp::endpoint endpoint_type;

        typedef listener_impl <
                bip::tcp::acceptor,
                bip::tcp::endpoint,
                connection_type
        > super_type;

        struct listener_tcp: public super_type {

            static endpoint_type make_endpoint( const std::string &address,
                                                unsigned short port )
            {
                return endpoint_type(bip::address::from_string(address), port);
            }

            listener_tcp( application &app,
                          const listener_options &opts,
                          const std::string &address,
                          unsigned short port )
                :super_type( app, opts, make_endpoint(address, port))
            { }

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

        listener *create(application &app, const listener_options &opts,
                            const std::string &address, unsigned short service)
        {
            return new listener_tcp( app, opts, address, service );
        }

        listener *create( application &app,
                                const std::string &address,
                                unsigned short service )
        {
            const listener_options def_opts(default_options( ));

            return create( app, def_opts, address, service );
        }

    }

}}}
