#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <sstream>
#include <algorithm>

#include "vtrc-endpoint-iface.h"
#include "vtrc-application-iface.h"
#include "vtrc-connection-iface.h"

#include "protocol/vtrc-auth.pb.h"
#include "vtrc-common/vtrc-sizepack-policy.h"
#include "vtrc-common/vtrc-enviroment.h"

namespace vtrc { namespace server { namespace endpoints {

    namespace {

        namespace basio = boost::asio;
        namespace bsys  = boost::system;

        namespace bip   = basio::ip;

        struct tcp_connection: public connection_iface {

            endpoint_iface       &endpoint_;
            application_iface    &app_;
            basio::io_service    &ios_;

            common::enviroment    env_;

            boost::shared_ptr<bip::tcp::socket> sock_;

            tcp_connection(endpoint_iface &endpoint, bip::tcp::socket *sock)
                :endpoint_(endpoint)
                ,app_(endpoint_.application( ))
                ,ios_(app_.get_io_service())
                ,sock_(sock)
            {}

            bool ready( ) const
            {
                sock_->is_open( );
            }

            endpoint_iface &endpoint( )
            {
                return endpoint_;
            }

            const char *name( ) const
            {
                return "<null>";
            }

            void close( )
            {
                sock_->close( );
            }

            common::enviroment &get_enviroment( )
            {
                return env_;
            }
        };

        struct endpoint_tcp: public endpoint_iface {

            typedef endpoint_tcp this_type;

            application_iface    &app_;
            basio::io_service    &ios_;

            bip::tcp::endpoint    endpoint_;
            bip::tcp::acceptor    acceptor_;

            endpoint_tcp( application_iface &app,
                          const std::string &address,
                          unsigned short port )
                :app_(app)
                ,ios_(app.get_io_service( ))
                ,endpoint_(bip::address::from_string(address), port)
                ,acceptor_(ios_, endpoint_)
            {}

            application_iface &application( )
            {
                return app_;
            }

            std::string string( ) const
            {
                std::ostringstream oss;
                oss << "tcp/" << endpoint_.address( ).to_string( )
                              << ":" << endpoint_.port( );
                return oss.str( );
            }

            void start_accept(  )
            {
                bip::tcp::socket *new_sock = new bip::tcp::socket(ios_);
                acceptor_.async_accept( *new_sock,
                    boost::bind( &this_type::on_accept, this,
                                 basio::placeholders::error, new_sock ));
            }

            void start( )
            {
                start_accept( );
                app_.on_endpoint_started( this );
            }

            void stop ( )
            {
                app_.on_endpoint_stopped( this );
            }

            void on_accept( const bsys::error_code &error,
                            bip::tcp::socket *sock )
            {
                if( error ) {
                    delete sock;
                } else {
                    start_accept( );
                }
            }

        };


    }

    namespace tcp {
        endpoint_iface *create( application_iface &app,
                                const std::string &address,
                                unsigned short service )
        {
            return new endpoint_tcp( app, address, service );
        }
    }

}}}
