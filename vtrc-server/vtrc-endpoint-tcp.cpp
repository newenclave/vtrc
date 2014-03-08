#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <sstream>
#include <deque>
#include <algorithm>

#include "vtrc-memory.h"

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

namespace vtrc { namespace server { namespace endpoints {

    namespace {

        namespace basio = boost::asio;
        namespace bsys  = boost::system;

        namespace bip   = basio::ip;

        struct tcp_connection: public common::transport_tcp {

            typedef tcp_connection this_type;

            endpoint_iface                     &endpoint_;
            application                        &app_;
            basio::io_service                  &ios_;
            common::enviroment                  env_;

            std::vector<char>                   read_buff_;

            shared_ptr<protocol_layer_s>        protocol_;

            tcp_connection(endpoint_iface &endpoint, bip::tcp::socket *sock)
                :common::transport_tcp(sock)
                ,endpoint_(endpoint)
                ,app_(endpoint_.get_application( ))
                ,ios_(app_.get_io_service( ))
                ,env_(endpoint_.get_enviroment( ))
                ,read_buff_(4096)
            {
                protocol_ = make_shared<server::protocol_layer_s>
                                                     (boost::ref(app_), this);
            }

            static shared_ptr<tcp_connection> create
                             (endpoint_iface &endpoint, bip::tcp::socket *sock)
            {
                shared_ptr<tcp_connection> new_inst
                                    (make_shared<tcp_connection>
                                                  (boost::ref(endpoint), sock));
                new_inst->init( );
                return new_inst;
            }

            void init( )
            {
                start_reading( );
                protocol_ ->init( );
            }

            bool ready( ) const
            {
                protocol_->ready( );
            }

            endpoint_iface &endpoint( )
            {
                return endpoint_;
            }

            std::string prepare_for_write( const char *data, size_t length )
            {
                return protocol_->prepare_data( data, length );
            }

            void on_write_error( const bsys::error_code & /*error*/ )
            {
                close( );
                //app_.get_clients( )->drop(this); // delete
            }

            void start_reading( )
            {
                get_socket( ).async_read_some(
                        basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                        boost::bind( &this_type::read_handler, this,
                             basio::placeholders::error,
                             basio::placeholders::bytes_transferred)
                    );
            }

            void read_handler( const bsys::error_code &error, size_t bytes )
            {
                if( !error ) {
                    try {
                        protocol_->process_data( &read_buff_[0], bytes );
                    } catch( const std::exception & /*ex*/ ) {
                        close( );
                        app_.get_clients( )->drop(this); // delete
                        return;
                    }
                    start_reading( );
                } else {
                    close( );
                    app_.get_clients( )->drop(this); // delete
                }
            }

            protocol_layer_s &get_protocol( )
            {
                return *protocol_;
            }
        };

        struct endpoint_tcp: public endpoint_iface {

            typedef endpoint_tcp this_type;

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
                    try {
                        std::cout << "accept\n";
                        shared_ptr<tcp_connection> new_conn
                                         (tcp_connection::create(*this, sock));
                        app_.get_clients( )->store( new_conn );
                        app_.on_new_connection_ready( new_conn.get( ) );
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
