#ifndef VTRC_ENDPOINT_IMPL_H
#define VTRC_ENDPOINT_IMPL_H

#include "vtrc-endpoint-iface.h"
#include "vtrc-application.h"
#include "vtrc-connection-iface.h"
#include "vtrc-connection-list.h"

#include "vtrc-ref.h"
#include "vtrc-bind.h"
#include "vtrc-atomic.h"
#include "vtrc-common/vtrc-closure.h"

#include "vtrc-common/vtrc-enviroment.h"

namespace vtrc { namespace server { namespace endpoints {

namespace {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;

    template <typename AcceptorType,
              typename EndpointType,
              typename TransportType>
    struct endpoint_impl: public endpoint_iface {

        typedef TransportType                         transport_type;
        typedef AcceptorType                          acceptor_type;
        typedef EndpointType                          endpoint_type;
        typedef typename transport_type::socket_type  socket_type;

        typedef vtrc::shared_ptr< vtrc::atomic<size_t> > shared_counter_type;

        typedef endpoint_impl<
                acceptor_type,
                endpoint_type,
                transport_type
        > this_type;

        application             &app_;
        basio::io_service       &ios_;
        common::enviroment       env_;

        endpoint_type            endpoint_;
        acceptor_type            acceptor_;

        endpoint_options         opts_;
        shared_counter_type      client_count_;

        endpoint_impl( application &app,
                      const endpoint_options &opts, const endpoint_type &ep)
            :app_(app)
            ,ios_(app_.get_io_service( ))
            ,env_(app_.get_enviroment())
            ,endpoint_(ep)
            ,acceptor_(ios_, endpoint_)
            ,opts_(opts)
            ,client_count_(vtrc::make_shared<vtrc::atomic<size_t> >(0))
        {}

        virtual ~endpoint_impl( ) { }

        static void on_client_destroy( shared_counter_type count )
        {
            --(*count);
        }

        size_t client_count( ) const
        {
            return (*client_count_);
        }

        common::empty_closure_type get_on_destroy( )
        {
            return vtrc::bind( &this_type::on_client_destroy, client_count_ );
        }

        application &get_application( )
        {
            return app_;
        }

        common::enviroment &get_enviroment( )
        {
            return env_;
        }

        virtual std::string string( ) const
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
            app_.on_endpoint_started( this );
        }

        void stop ( )
        {
            app_.on_endpoint_stopped( this );
            acceptor_.close( );
        }

        virtual const endpoint_options &get_options( ) const
        {
            return opts_;
        }

        void on_accept( const bsys::error_code &error,
                        vtrc::shared_ptr<socket_type> sock )
        {
            if( !error ) {
                try {
                    vtrc::shared_ptr<transport_type> new_conn
                           (transport_type::create( *this, sock,
                                                    get_on_destroy( )));
                    app_.get_clients( )->store( new_conn );
                    ++(*client_count_);
                } catch( ... ) {
                    ;;;
                }
                start_accept( );
            } else {
                //delete sock;
            }
        }

    };
}

}}}

#endif // VTRC_ENDPOINT_IMPL_H
