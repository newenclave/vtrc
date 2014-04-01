#include "vtrc-endpoint-win-pipe.h"

#ifdef _WIN32

#include <boost/asio.hpp>

#include "vtrc-application.h"
#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-endpoint-iface.h"
#include "vtrc-connection-impl.h"

namespace vtrc { namespace server { namespace endpoints {

namespace {

    namespace basio   = boost::asio;

    struct pipe_ep_impl: public endpoint_iface {

        application             &app_;
        basio::io_service       &ios_;
        common::enviroment       env_;

        endpoint_options         opts_;

        pipe_ep_impl( application &app,
                const endpoint_options &opts, 
                const std::string pipe_name)
            :app_(app)
            ,ios_(app_.get_io_service( ))
            ,env_(app_.get_enviroment())
            ,opts_(opts)
        {}

        virtual ~pipe_ep_impl( ) { }

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

        virtual const endpoint_options &get_options( ) const
        {
            return opts_;
        }

        //void on_accept( const bsys::error_code &error,
        //                socket_type* sock )
        //{
        //    if( !error ) {
        //        try {
        //            vtrc::shared_ptr<transport_type> new_conn
        //                     (transport_type::create( *this, sock ));
        //            app_.get_clients( )->store( new_conn );
        //        } catch( ... ) {
        //            ;;;
        //        }
        //        start_accept( );
        //    } else {
        //        delete sock;
        //    }
        //}

    };

}

}}}

#endif
