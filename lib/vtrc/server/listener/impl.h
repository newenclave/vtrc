#ifndef VTRC_ENDPOINT_IMPL_H
#define VTRC_ENDPOINT_IMPL_H

#include "vtrc-asio.h"

#include "vtrc/server/listener.h"
#include "vtrc/server/application.h"

#include "vtrc-ref.h"
#include "vtrc-bind.h"
#include "vtrc-thread.h"

#include "vtrc/common/connection-iface.h"
#include "vtrc/common/connection-list.h"
#include "vtrc/common/closure.h"
#include "vtrc/common/environment.h"
#include "vtrc/common/monotonic-timer.h"

#include "vtrc/server/connection/closure.h"

namespace vtrc { namespace server { namespace listeners {

namespace {

    namespace basio = VTRC_ASIO;
    namespace bsys  = VTRC_SYSTEM;

    typedef vtrc::server::connection_close_closure close_closure;

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
                       const rpc::session_options &opts,
                       const endpoint_type &ep)
            :listener(app, opts)
            ,ios_(app.get_io_service( ))
            ,endpoint_(ep)
            ,working_(false)
        { }

        virtual ~listener_impl( ) { }

        void on_client_destroy( vtrc::weak_ptr<listener> inst,
                                common::connection_iface *conn )
        {
            vtrc::shared_ptr<listener> lock( inst.lock( ) );
            if( lock ) {
                this->stop_connection( conn );
            }
        }

        close_closure get_on_close_cb( )
        {
            return vtrc::bind( &this_type::on_client_destroy, this,
                               weak_from_this( ), vtrc::placeholders::_1 );
        }

        virtual std::string name( ) const
        {
            return "unknown://";
        }

        virtual native_handle_type native_handle( ) const
        {
            native_handle_type res;
#ifdef _WIN32
            res.value.win_handle = reinterpret_cast<native_handle_type::handle>
                    ((SOCKET)acceptor_->native_handle( ));
#else
            res.value.unix_fd = acceptor_->native_handle( );
#endif
            return res;
        }

        void start_accept(  )
        {

            vtrc::shared_ptr<socket_type> new_sock
                    ( vtrc::make_shared<socket_type>(vtrc::ref(ios_) ) );

            acceptor_->async_accept( *new_sock,
                vtrc::bind( &this_type::on_accept, this,
                             vtrc::placeholders::error, new_sock,
                             weak_from_this( )));
        }

        acceptor_type *acceptor( ) const
        {
            return acceptor_.get( );
        }

        void start( )
        {
            acceptor_.reset(new acceptor_type(ios_, endpoint_));
            acceptor_->listen( 5 );
            start_accept( );
            working_ = true;
            start_success( );
            call_on_start( );
        }

        virtual void start_success( )
        { }

        void stop ( )
        {
            working_ = false;
            acceptor_->close( );
            stop_impl( );
        }

        bool is_active( ) const
        {
            return working_;
        }

        virtual void stop_impl( )
        {

        }

        virtual void connection_setup( vtrc::shared_ptr<connection_type> &con )
        {

        }

        void on_accept( const bsys::error_code &error,
                        vtrc::shared_ptr<socket_type> sock,
                        vtrc::weak_ptr<listener> &inst )
        {

            vtrc::shared_ptr<listener> locked( inst.lock( ));
            if( !locked ) {
                return;
            }

            if( !error ) {
                vtrc::shared_ptr<connection_type> new_conn;
                try {
                    new_conn = connection_type::create( *this, sock,
                                                         get_on_close_cb( ));

                    connection_setup( new_conn );

                    namespace ph = vtrc::placeholders;

                    new_conn->get_protocol( )
                             .assign_lowlevel_factory(
                                lowlevel_protocol_factory( ) );

                    get_application( ).get_clients( )->store( new_conn );
                    new_connection( new_conn.get( ) );

                    new_conn->init( );

//                    get_application( ).get_io_service( )
//                        .dispatch( vtrc::bind( &connection_type::init,
//                                                new_conn ) );

                } catch( ... ) {
                    //sock->close( );
                    if( new_conn.get( ) )  {
                        get_application( ).get_clients( )
                                         ->drop( new_conn.get( ) );
                    }
                }
                start_accept( );
            } else {
                if( working_ ) {
                    stop( );
                    call_on_accept_failed( error );
                } else {
                    call_on_stop( );
                }
            }
        }

    };
}

}}}

#endif // VTRC_ENDPOINT_IMPL_H
