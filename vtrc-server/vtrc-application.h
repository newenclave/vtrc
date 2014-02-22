#ifndef VTRC_APPLICATION_IFACE_H
#define VTRC_APPLICATION_IFACE_H

#include <boost/shared_ptr.hpp>

namespace boost { namespace asio {
    class io_service;
}}

namespace google { namespace protobuf {
    class Service;
}}

namespace vtrc {

    namespace common {
        struct connection_iface;
        class enviroment;
    }

namespace server {

    struct endpoint_iface;
    struct connection_iface;
    class  connection_list;

    class application {

        struct application_impl;
        friend struct application_impl;
        application_impl  *impl_;

    public:

        application( );
        application( boost::asio::io_service &ios );
        virtual ~application( );

        common::enviroment      &get_enviroment( );
        boost::asio::io_service &get_io_service( );

        boost::shared_ptr<connection_list> get_clients( );

        /** TODO: fix it **/
        virtual void on_endpoint_started(   endpoint_iface * /*ep*/ ) { }
        virtual void on_endpoint_stopped(   endpoint_iface * /*ep*/ ) { }
        virtual void on_endpoint_exception( endpoint_iface * /*ep*/ ) { }

        virtual void on_new_connection_accepted(
                common::connection_iface* connection ) { };

        virtual void on_new_connection_ready(
                            common::connection_iface* connection ) { };

        virtual void on_connection_die(
                            common::connection_iface* connection ) { };

        /** TODO: fix it **/

    };

}}

#endif
