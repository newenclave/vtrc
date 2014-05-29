#ifndef VTRC_ENDPOINT_IMPL_H
#define VTRC_ENDPOINT_IMPL_H

#include "boost/asio.hpp"

#include "vtrc-listener.h"
#include "vtrc-application.h"

#include "vtrc-connection-iface.h"
#include "vtrc-connection-list.h"

#include "vtrc-ref.h"
#include "vtrc-bind.h"
#include "vtrc-common/vtrc-closure.h"

#include "vtrc-common/vtrc-enviroment.h"

#include "vtrc-thread.h"
#include "vtrc-common/vtrc-monotonic-timer.h"

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

        basio::io_service              &ios_;

        endpoint_type                   endpoint_;
        vtrc::unique_ptr<acceptor_type> acceptor_;
        bool                            working_;

        listener_impl( application &app,
                       const vtrc_rpc::session_options &opts,
                       const endpoint_type &ep)
            :listener(app, opts)
            ,ios_(app.get_io_service( ))
            ,endpoint_(ep)
            ,working_(false)
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

        close_closure get_on_close_cb( )
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

            acceptor_->async_accept( *new_sock,
                vtrc::bind( &this_type::on_accept, this,
                             basio::placeholders::error, new_sock ));
        }

        void start( )
        {
            working_ = true;
            acceptor_.reset(new acceptor_type(ios_, endpoint_));
            start_accept( );
            get_on_start( )( );
        }

        void stop ( )
        {
            working_ = false;
            acceptor_->close( );
            stop_impl( );
        }

        virtual void stop_impl( )
        {

        }

        virtual void connection_setup( vtrc::shared_ptr<connection_type> &con )
        {

        }

        void on_accept( const bsys::error_code &error,
                        vtrc::shared_ptr<socket_type> sock )
        {
            if( !error ) {
                vtrc::shared_ptr<connection_type> new_conn;
                try {
                    new_conn = connection_type::create( *this, sock,
                                                         get_on_close_cb( ));
                    connection_setup( new_conn );

                    get_application( ).get_clients( )->store( new_conn );
                    new_connection( new_conn.get( ) );

                } catch( ... ) {
                    sock->close( );
                    if( new_conn.get( ) )  {
                        get_application( )
                                .get_clients( )->drop( new_conn.get( ) );
                    }
                }
                start_accept( );
            } else {
                if( working_ ) {
                    stop( );
                    get_on_accept_failed(  )( error );
                } else {
                    get_on_stop( )( );
                }
            }
        }

    };
}

}}}

#endif // VTRC_ENDPOINT_IMPL_H
