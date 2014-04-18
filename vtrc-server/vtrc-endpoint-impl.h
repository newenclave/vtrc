#ifndef VTRC_ENDPOINT_IMPL_H
#define VTRC_ENDPOINT_IMPL_H

#include "boost/asio.hpp"

#include "vtrc-endpoint-base.h"
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
              typename ConnectionType>
    struct endpoint_impl: public endpoint_base {

        typedef ConnectionType                        connection_type;
        typedef AcceptorType                          acceptor_type;
        typedef EndpointType                          endpoint_type;
        typedef typename connection_type::socket_type socket_type;

        typedef vtrc::shared_ptr< vtrc::atomic<size_t> > shared_counter_type;

        typedef endpoint_impl<
                acceptor_type,
                endpoint_type,
                connection_type
        > this_type;

        basio::io_service       &ios_;

        endpoint_type            endpoint_;
        acceptor_type            acceptor_;

        shared_counter_type      client_count_;

        endpoint_impl( application &app,
                       const endpoint_options &opts, const endpoint_type &ep)
            :endpoint_base(app, opts)
            ,ios_(app.get_io_service( ))
            ,endpoint_(ep)
            ,acceptor_(ios_, endpoint_)
            ,client_count_(vtrc::make_shared<vtrc::atomic<size_t> >(0))
        { }

        virtual ~endpoint_impl( ) { }

        static void on_client_destroy( shared_counter_type count )
        {
            --(*count);
        }

        size_t clients_count( ) const
        {
            return (*client_count_);
        }

        common::empty_closure_type get_on_destroy( )
        {
            return vtrc::bind( &this_type::on_client_destroy, client_count_ );
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
            get_application( ).on_endpoint_started( this );
        }

        void stop ( )
        {
            get_application( ).on_endpoint_stopped( this );
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
