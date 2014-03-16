#ifndef VTRC_APPLICATION_IFACE_H
#define VTRC_APPLICATION_IFACE_H

#include "vtrc-memory.h"

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
        class rpc_service_wrapper;
    }

namespace server {

    struct endpoint_iface;
    struct connection_iface;
    class  connection_list;

    class application {

        struct impl;
        friend struct impl;
        impl  *impl_;

        application( const application & );
        application& operator = ( const application & );

    public:

        application( );
        application( boost::asio::io_service &ios );
        virtual ~application( );

        common::enviroment      &get_enviroment( );
        boost::asio::io_service &get_io_service( );

        vtrc::shared_ptr<connection_list>      get_clients( );

        virtual vtrc::shared_ptr<common::rpc_service_wrapper>
                 get_service_by_name( common::connection_iface* connection,
                                      const std::string &service_name );

        /** TODO: fix it **/
        virtual void on_endpoint_started(   endpoint_iface * /*ep*/ ) { }
        virtual void on_endpoint_stopped(   endpoint_iface * /*ep*/ ) { }
        virtual void on_endpoint_exception( endpoint_iface * /*ep*/ ) { }

        virtual void on_new_connection_accepted(
                common::connection_iface* connection ) { }

        virtual void on_new_connection_ready(
                            common::connection_iface* connection ) { }

        virtual void on_connection_die(
                            common::connection_iface* connection ) { }

        /** TODO: fix it **/

    };

}}

#endif
