#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <sstream>

#include "vtrc-endpoint-iface.h"
#include "vtrc-application-iface.h"

namespace vtrc { namespace server { namespace endpoints {
    
    namespace {
        
        namespace basio = boost::asio;
        namespace bsys  = boost::system;

        namespace bip   = basio::ip;

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
                    sock->close( );
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
