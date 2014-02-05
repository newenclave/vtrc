#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <sstream>
#include <deque>
#include <algorithm>

#include "vtrc-endpoint-iface.h"
#include "vtrc-application-iface.h"
#include "vtrc-connection-iface.h"

#include "protocol/vtrc-auth.pb.h"
#include "vtrc-common/vtrc-sizepack-policy.h"
#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-common/vtrc-hasher-iface.h"
#include "vtrc-common/vtrc-hasher-impls.h"
#include "vtrc-common/vtrc-data-queue.h"

#include "vtrc-protocol-layer.h"

namespace vtrc { namespace server { namespace endpoints {

    namespace {

        namespace basio = boost::asio;
        namespace bsys  = boost::system;

        namespace bip   = basio::ip;

        struct tcp_connection: public connection_iface {

            typedef tcp_connection this_type;

            endpoint_iface                     &endpoint_;
            application_iface                  &app_;
            basio::io_service                  &ios_;

            basio::io_service::strand           write_dispatcher_;
            std::deque<std::string>             write_queue_;

            common::enviroment                  env_;

            boost::shared_ptr<bip::tcp::socket> sock_;
            std::string                         name_;

            std::vector<char>                   read_buff_;

            boost::shared_ptr<common::hasher_iface> hasher_;
            common::data_queue::queue_base_sptr     queue_;

            boost::shared_ptr<protocol_layer>       protocol_;

            tcp_connection(endpoint_iface &endpoint, bip::tcp::socket *sock)
                :endpoint_(endpoint)
                ,app_(endpoint_.application( ))
                ,ios_(app_.get_io_service( ))
                ,write_dispatcher_(ios_)
                ,sock_(sock)
                ,read_buff_(4096)
                ,hasher_(common::hasher::fake::create( ))
                ,queue_(common::data_queue::varint::create_parser( 16 * 1024 ))
            {
                protocol_ = boost::make_shared<protocol_layer>(this);
                start_reading( );
                protocol_ ->init( );
            }

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

            boost::asio::io_service &get_io_service( )
            {
                return ios_;
            }

            void write( const char *data, size_t length )
            {
                write_dispatcher_.post(
                       boost::bind( &this_type::write_impl, this,
                                    std::string( data, data + length )));
            }

            void async_write( )
            {
                sock_->async_send(
                        basio::buffer( write_queue_.front( ) ),
                        write_dispatcher_.wrap(
                                boost::bind( &this_type::write_handler, this,
                                     basio::placeholders::error,
                                     basio::placeholders::bytes_transferred ))
                        );
            }

            void write_impl( const std::string data )
            {
                bool empty = write_queue_.empty( );
                write_queue_.push_back( data );

                if( empty ) {
                    async_write( );
                }
            }

            void write_handler( const bsys::error_code &error, size_t bytes )
            {
                if( !error ) {
                    write_queue_.pop_front( );
                    if( !write_queue_.empty( ) )
                        async_write( );
                } else {
                    close( );
                    app_.on_connection_die( this ); // delete
                }
            }

            void start_reading( )
            {
                sock_->async_read_some(
                        basio::buffer( &read_buff_[0], read_buff_.size() ),
                        boost::bind( &this_type::read_handler, this,
                             basio::placeholders::error,
                             basio::placeholders::bytes_transferred)
                    );
            }

            void read_handler( const bsys::error_code &error, size_t bytes )
            {
                if( !error ) {
                    std::cout << "got " << bytes << "\n";
                    protocol_->read_data( &read_buff_[0], bytes );
                    start_reading( );
                } else {
                    close( );
                    app_.on_connection_die( this ); // delete
                }
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
                    try {
                        tcp_connection *new_conn
                                    (new tcp_connection(*this, sock));
                        app_.on_new_connection_accepted( new_conn );
                        app_.on_new_connection_ready( new_conn );
                    } catch( ... ) {
                        ;;;
                    }
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
