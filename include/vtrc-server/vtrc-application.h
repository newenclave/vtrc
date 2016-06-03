#ifndef VTRC_APPLICATION_IFACE_H
#define VTRC_APPLICATION_IFACE_H

#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-protocol-defaults.h"
#include "vtrc-common/vtrc-protocol-iface.h"

#include "vtrc-asio-forward.h"

#include "vtrc-function.h"

VTRC_ASIO_FORWARD( class io_service; )

namespace google { namespace protobuf {
    class Service;
}}

namespace vtrc { namespace rpc {
    namespace errors {
        class container;
    }
    class session_options;
    class lowlevel_unit;
}}

namespace vtrc {

    namespace common {
        struct connection_iface;
        class  enviroment;
        class  connection_list;
        class  pool_pair;
        struct connection_iface;
    }

namespace server {

    typedef vtrc::function<
        vtrc::shared_ptr<google::protobuf::Service> ( common::connection_iface*,
                                                      const std::string & )
    > service_factory_type;

    class application {

        struct         impl;
        friend struct  impl;
        impl          *impl_;

        application( const application & );
        application& operator = ( const application & );

    public:

        application( );
        application( common::pool_pair &pools );
        application( VTRC_ASIO::io_service &ios );
        application( VTRC_ASIO::io_service &ios,
                     VTRC_ASIO::io_service &rpc_ios );

        virtual ~application( );

        void stop_all_clients( );

        common::enviroment          &get_enviroment ( );
        VTRC_ASIO::io_service       &get_io_service ( );
        VTRC_ASIO::io_service       &get_rpc_service( );

        const common::enviroment    &get_enviroment ( ) const;
        const VTRC_ASIO::io_service &get_io_service ( ) const;
        const VTRC_ASIO::io_service &get_rpc_service( ) const;

        vtrc::shared_ptr<common::connection_list>  get_clients( );

        void assign_service_factory( service_factory_type factory );

        virtual void execute( common::protocol_iface::call_type call );

        virtual void configure_session( common::connection_iface* connection,
                                        rpc::session_options &opts );

        virtual vtrc::shared_ptr<common::rpc_service_wrapper>
                 get_service_by_name( common::connection_iface* connection,
                                      const std::string &service_name );

        virtual std::string get_session_key( common::connection_iface* conn,
                                             const std::string &id );

        virtual bool session_key_required( common::connection_iface* conn,
                                           const std::string &id );
    };

}}

#endif
