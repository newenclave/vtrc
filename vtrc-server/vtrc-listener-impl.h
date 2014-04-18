#ifndef VTRC_ENDPOINT_IMPL_H
#define VTRC_ENDPOINT_IMPL_H

#include "boost/asio.hpp"

#include "vtrc-listener.h"
#include "vtrc-application.h"

#include "vtrc-connection-iface.h"
#include "vtrc-connection-list.h"

#include "vtrc-ref.h"
#include "vtrc-bind.h"
#include "vtrc-atomic.h"
#include "vtrc-common/vtrc-closure.h"

#include "vtrc-common/vtrc-enviroment.h"

namespace vtrc { namespace server { namespace listeners {

namespace {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;

    typedef vtrc::function<
        void (const common::connection_iface *)
    > close_closure;

    template <typename AcceptorType,
              typename EndpointType,
              typename ConnectionType>
    struct listener_impl: public listener {

        typedef ConnectionType                        connection_type;
        typedef AcceptorType                          acceptor_type;
        typedef EndpointType                          endpoint_type;
        typedef typename connection_type::socket_type socket_type;

        typedef listener_impl<
                acceptor_type,
                endpoint_type,
                connection_type
        > this_type;

        basio::io_service       &ios_;

        endpoint_type            endpoint_;
        acceptor_type            acceptor_;

        listener_impl( application &app,
                       const listener_options &opts, const endpoint_type &ep)
            :listener(app, opts)
            ,ios_(app.get_io_service( ))
            ,endpoint_(ep)
            ,acceptor_(ios_, endpoint_)
        { }

        virtual ~listener_impl( ) { }

        void on_client_destroy( vtrc::weak_ptr<listener> inst,
                                const common::connection_iface *conn )
        {
            vtrc::shared_ptr<listener> lock( inst.lock( ) );
            if( lock ) {
                this->stop_connection( conn );
            }
        }

        close_closure get_on_destroy( )
        {
            return vtrc::bind( &this_type::on_client_destroy, this,
                               weak_from_this( ), _1 );
        }

        virtual std::string name( ) const
        {
            return "unknown://";
        }

        void start_accept(  )
        {
            vtrc::shared_ptr<socket_type> new_sock
                    ( vtrc::make_shared<socket_type>(vtrc::ref(ios_) ) );

            acceptor_.async_accept( *new_sock,
                vtrc::bind( &this_type::on_accept, this,
                             basio::placeholders::error, new_sock ));
        }

        void start( )
        {
            start_accept( );
            on_start_( );
        }

        void stop ( )
        {
            acceptor_.close( );
        }

        void on_accept( const bsys::error_code &error,
                        vtrc::shared_ptr<socket_type> sock )
        {
            if( !error ) {
                try {

                    vtrc::shared_ptr<connection_type> new_conn
                           (connection_type::create( *this, sock,
                                                    get_on_destroy( )));

                    get_application( ).get_clients( )->store( new_conn );
                    new_connection( new_conn.get( ) );

                } catch( ... ) {
                    ;;;
                }
                start_accept( );
            } else {
                on_stop_( );
                //delete sock;
            }
        }

    };
}

}}}

#endif // VTRC_ENDPOINT_IMPL_H
