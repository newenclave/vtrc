#ifndef VTRC_CLIENT_BASE_H
#define VTRC_CLIENT_BASE_H

#include "vtrc-memory.h"
#include "vtrc-function.h"
#include "vtrc-asio-forward.h"
#include "vtrc-system-forward.h"

#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-protocol-iface.h"

namespace vtrc {

namespace rpc {
    class lowlevel_unit;
}

namespace rpc { namespace errors {
    class container;
}}

namespace client {

    //// IS NOT YET COMPLETE
    class base: public vtrc::enable_shared_from_this<base> {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        base( const base &other );
        base & operator = ( const base &other );

        VTRC_DECLARE_SIGNAL( on_init_error,
                             void( const rpc::errors::container &,
                                   const char *message ) );

        VTRC_DECLARE_SIGNAL( on_connect,    void( ) );

        VTRC_DECLARE_SIGNAL( on_disconnect, void( ) );
        VTRC_DECLARE_SIGNAL( on_ready,      void( ) );

    protected:

        base( VTRC_ASIO::io_service &ios );
        base( VTRC_ASIO::io_service &ios,
              VTRC_ASIO::io_service &rpc_ios );
        base( common::pool_pair &pools );

        void reset_connection( common::connection_iface_sptr new_conn );
        common::protocol_iface *init_protocol(common::connection_iface_sptr c );

    public:

        virtual void connect( ) = 0;
        virtual void async_connect( common::system_closure_type ) = 0;

        common::connection_iface_sptr connection( );

    public:

        vtrc::weak_ptr<base>         weak_from_this( );
        vtrc::weak_ptr<base const>   weak_from_this( ) const;

        VTRC_ASIO::io_service       &get_io_service( );
        const VTRC_ASIO::io_service &get_io_service( ) const;

        VTRC_ASIO::io_service       &get_rpc_service( );
        const VTRC_ASIO::io_service &get_rpc_service( ) const;

        common::rpc_channel         *create_channel( );
        common::rpc_channel         *create_channel( unsigned flags );

        bool ready( ) const;
        void disconnect( );

    public: //// services

        typedef google::protobuf::Service               service_type;
        typedef vtrc::shared_ptr<service_type>          service_sptr;
        typedef vtrc::weak_ptr<service_type>            service_wptr;

        typedef common::rpc_service_wrapper             service_wrapper_type;
        typedef vtrc::shared_ptr<service_wrapper_type>  service_wrapper_sptr;
        typedef vtrc::weak_ptr<service_wrapper_type>    service_wrapper_wptr;

        typedef vtrc::function<
                    common::rpc_service_wrapper_sptr (const std::string &)
                > service_factory_type;
        typedef common::lowlevel::protocol_factory_type lowlevel_factory_type;

        typedef common::protocol_iface::executor_type executor_type;

        service_wrapper_sptr wrap_service( service_sptr svc )
        {
            return vtrc::make_shared<service_wrapper_type>( svc );
        }

        service_wrapper_sptr wrap_service( service_type *svc )
        {
            return vtrc::make_shared<service_wrapper_type>( svc );
        }

        void assign_rpc_handler( service_sptr handler);
        void assign_weak_rpc_handler( service_wptr handler);

        void assign_rpc_handler( service_wrapper_sptr handler );
        void assign_weak_rpc_handler( service_wrapper_wptr handler);

        void assign_service_factory( service_factory_type factory );

        service_wrapper_sptr get_rpc_handler( const std::string &name );

        void erase_rpc_handler( service_sptr handler);

        void erase_rpc_handler( const std::string &name );
        void erase_all_rpc_handlers( );

        //// low level factory
        ///
        void assign_lowlevel_protocol_factory( lowlevel_factory_type factory );
        lowlevel_factory_type lowlevel_protocol_factory( );

        /// executor
        ///
        void assign_call_executor( executor_type exec );
        executor_type call_executor( );
        void execute( common::protocol_iface::call_type call );

    };
}}

#endif // VTRC_CLIENT_BASE_H
