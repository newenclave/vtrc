#ifndef VTRC_APPLICATION_IFACE_H
#define VTRC_APPLICATION_IFACE_H

#include <boost/function.hpp>

namespace boost { namespace asio {
    class io_service;  
}}

namespace vtrc { namespace server {

    struct endpoint_iface;

    struct application_iface {
        virtual ~application_iface( ) { }
        virtual boost::asio::io_service &get_io_service( ) = 0;

        // events
        virtual void on_endpoint_started( endpoint_iface * /*ep*/ ) { }
        virtual void on_endpoint_stopped( endpoint_iface * /*ep*/ ) { }
        virtual void on_endpoint_exception( endpoint_iface * /*ep*/ ) { }
    };

}}

#endif
