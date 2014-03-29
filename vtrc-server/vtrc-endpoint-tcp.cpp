#include <boost/asio.hpp>
#include <sstream>
#include <deque>
#include <algorithm>

#include "vtrc-endpoint-iface.h"
#include "vtrc-application.h"
#include "vtrc-connection-iface.h"
#include "vtrc-connection-list.h"

#include "protocol/vtrc-auth.pb.h"

#include "vtrc-common/vtrc-sizepack-policy.h"
#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-common/vtrc-hash-iface.h"
#include "vtrc-common/vtrc-data-queue.h"

#include "vtrc-common/vtrc-transport-tcp.h"

#include "vtrc-protocol-layer-s.h"

#include "vtrc-connection-impl.h"

namespace vtrc { namespace server { namespace endpoints {

    namespace {

        namespace basio = boost::asio;
        namespace bsys  = boost::system;

        namespace bip   = basio::ip;

        struct endpoint_tcp: public endpoint_iface {

            typedef endpoint_tcp this_type;

            typedef common::transport_tcp           transport_type;
            typedef connection_impl<transport_type> connection_type;
            typedef connection_type::socket_type    socket_type;

            application             &app_;
            basio::io_service       &ios_;
            common::enviroment       env_;

            bip::tcp::endpoint       endpoint_;
            bip::tcp::acceptor       acceptor_;

            endpoint_tcp( application &app,
                          const std::string &address,
                          unsigned short port )
                :app_(app)
                ,ios_(app_.get_io_service( ))
                ,env_(app_.get_enviroment())
                ,endpoint_(bip::address::from_string(address), port)
                ,acceptor_(ios_, endpoint_)
            {}

            application &get_application( )
            {
                return app_;
            }

            common::enviroment &get_enviroment( )
            {
                return env_;
            }

            std::string string( ) const
            {
                std::ostringstream oss;
                oss << "tcp://" << endpoint_.address( ).to_string( )
                              << ":" << endpoint_.port( );
                return oss.str( );
            }

            void start_accept(  )
            {
                bip::tcp::socket *new_sock = new bip::tcp::socket(ios_);
                acceptor_.async_accept( *new_sock,
                    vtrc::bind( &this_type::on_accept, this,
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
                acceptor_.close( );
            }

            void on_accept( const bsys::error_code &error,
                            bip::tcp::socket *sock )
            {
                if( error ) {
                    delete sock;
                } else {
                    try {
                        std::cout << "accept\n";
                        vtrc::shared_ptr<connection_type> new_conn
                                 (connection_type::create(*this, sock, 4096));
                        app_.get_clients( )->store( new_conn );
                    } catch( ... ) {
                        ;;;
                    }
                    start_accept( );
                }
            }

        };


    }

    namespace tcp {
        endpoint_iface *create( application &app,
                                const std::string &address,
                                unsigned short service )
        {
            return new endpoint_tcp( app, address, service );
        }
    }

}}}
