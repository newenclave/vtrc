#ifndef VTRC_APPLICATION_IFACE_H
#define VTRC_APPLICATION_IFACE_H

#include <boost/shared_ptr.hpp>

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc { namespace server {

    struct endpoint_iface;
    struct connection_iface;

    struct application_iface {

        virtual ~application_iface( ) { }
        virtual boost::asio::io_service &get_io_service( ) = 0;

        // events
        virtual void on_endpoint_started( endpoint_iface * /*ep*/ ) { }
        virtual void on_endpoint_stopped( endpoint_iface * /*ep*/ ) { }
        virtual void on_endpoint_exception( endpoint_iface * /*ep*/ ) { }

        virtual void on_new_connection_accepted(
                           boost::shared_ptr<connection_iface> connection ) = 0;

        virtual void on_new_connection_ready(
                            const connection_iface* connection ) = 0;

        virtual void on_connection_die(
                            connection_iface* connection ) = 0;

    };

}}

#endif
