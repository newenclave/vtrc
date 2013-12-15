#include <boost/asio.hpp>

#include "vtrc-endpoint-iface.h"
#include "vtrc-application-iface.h"

namespace vtrc { namespace server { namespace endpoints {
    
    namespace {
        
        namespace basio = boost::asio;

        struct endpoint_tcp: public endpoint_iface {

            application_iface &app_;
            basio::io_service &ios_;

            endpoint_tcp( application_iface &app )
                :app_(app)
                ,ios_(app.get_io_service( ))
            {}

            application_iface &application( )
            {
                return app_;
            }

            std::string string( ) const
            {
                return "tcp/";
            }

            void start( )
            {
                app_.on_endpoint_started( this );
            }

            void stop ( )
            {
                app_.on_endpoint_stopped( this );
            }
        };
    }

    namespace tcp {
        endpoint_iface *create( application_iface &app ) 
        {
            return new endpoint_tcp( app );
        }
    }

}}}
