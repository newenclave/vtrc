
#ifndef  _WIN32

#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>

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

#include "vtrc-common/vtrc-transport-unix-local.h"

#include "vtrc-protocol-layer-s.h"

#include "vtrc-connection-impl.h"

namespace vtrc { namespace server { namespace endpoints {

    namespace {

        namespace basio   = boost::asio;
        namespace bsys    = boost::system;

        namespace blocal  = basio::local;

        typedef blocal::stream_protocol bstream;

        struct endpoint_unix: public endpoint_iface {

            typedef endpoint_unix this_type;

            typedef common::transport_unix_local    transport_type;
            typedef connection_impl<transport_type> connection_type;
            typedef connection_type::socket_type    socket_type;

            application             &app_;
            basio::io_service       &ios_;
            common::enviroment       env_;

            bstream::endpoint       endpoint_;
            bstream::acceptor       acceptor_;

            endpoint_unix( application &app, const std::string &name )
                :app_(app)
                ,ios_(app_.get_io_service( ))
                ,env_(app_.get_enviroment())
                ,endpoint_(name)
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
                oss << "unix://" << endpoint_.path( );
                return oss.str( );
            }

            void start_accept(  )
            {
                vtrc::shared_ptr<socket_type> new_sock
                        (vtrc::make_shared<socket_type>(vtrc::ref(ios_)));

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
                ::unlink(endpoint_.path( ).c_str( ));
            }

            void on_accept( const bsys::error_code &error,
                            vtrc::shared_ptr<socket_type> sock )
            {
                if( error ) {
                    //delete sock;
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

    namespace unix_local {
        endpoint_iface *create( application &app, const std::string &name )
        {
            ::unlink( name.c_str( ) );
            return new endpoint_unix( app, name );
        }
    }

}}}


#endif


