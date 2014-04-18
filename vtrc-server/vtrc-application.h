#ifndef VTRC_APPLICATION_IFACE_H
#define VTRC_APPLICATION_IFACE_H

//#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-memory.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace google { namespace protobuf {
    class Service;
}}

namespace vtrc_errors {
    class container;
}

namespace vtrc {

    namespace common {
        struct connection_iface;
        class enviroment;
        class rpc_service_wrapper;
        class connection_list;
        class pool_pair;
    }

namespace server {

    class  listener;
    struct connection_iface;

    class application {

        struct impl;
        friend struct impl;
        impl  *impl_;

        application( const application & );
        application& operator = ( const application & );

        friend class listener;

    public:

        application( );
        application( common::pool_pair &pools );
        application( boost::asio::io_service &ios );
        application( boost::asio::io_service &ios,
                     boost::asio::io_service &rpc_ios);
        virtual ~application( );

        void stop_all_clients( );

        common::enviroment      &get_enviroment( );
        boost::asio::io_service &get_io_service( );
        boost::asio::io_service &get_rpc_service( );

        vtrc::shared_ptr<common::connection_list>  get_clients( );

        virtual vtrc::shared_ptr<common::rpc_service_wrapper>
                 get_service_by_name( common::connection_iface* connection,
                                      const std::string &service_name );

        virtual std::string get_session_key( common::connection_iface* conn,
                                             const std::string &id);

        virtual void attach_listener( vtrc::shared_ptr<listener> nl );
    };

}}

#endif
