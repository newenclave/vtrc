#ifndef VTRC_ENDPOINT_IFACE_H
#define VTRC_ENDPOINT_IFACE_H

#include <string>

#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-protocol-defaults.h"
#include "vtrc-common/vtrc-lowlevel-protocol-iface.h"

#include "vtrc-memory.h"
#include "vtrc-system-forward.h"

namespace google { namespace protobuf {
    class MethodDescriptor;
}}

namespace vtrc { namespace rpc {
    class session_options;
    class lowlevel_unit;
}}

VTRC_SYSTEM_FORWARD( class error_code; )

namespace vtrc {

    namespace common {
        class  enviroment;
        struct connection_iface;
    }

namespace server {

    class application;

    class listener: public vtrc::enable_shared_from_this<listener> {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        VTRC_DECLARE_SIGNAL( on_start, void ( ) );
        VTRC_DECLARE_SIGNAL( on_stop,  void ( ) );

        VTRC_DECLARE_SIGNAL( on_accept_failed,
                             void ( const VTRC_SYSTEM::error_code &err ) );

        VTRC_DECLARE_SIGNAL( on_new_connection,
                             void ( common::connection_iface * ) );

        VTRC_DECLARE_SIGNAL( on_stop_connection,
                             void ( common::connection_iface * ) );


    protected:

        listener( application & app, const rpc::session_options &opts );

    public:

        virtual ~listener( );

    public:

        application                      &get_application( );
        common::enviroment               &get_enviroment( );
        const rpc::session_options       &get_options( ) const;
        size_t clients_count( ) const;

        vtrc::weak_ptr<listener>       weak_from_this( );
        vtrc::weak_ptr<listener const> weak_from_this( ) const;

    public:

        typedef common::lowlevel::protocol_factory_type lowlevel_factory_type;

        void set_precall( const common::precall_closure_type &func );
        void set_postcall( const common::postcall_closure_type &func );

        void assign_lowlevel_protocol_factory( lowlevel_factory_type factory );
        lowlevel_factory_type lowlevel_protocol_factory( );

        virtual std::string name( ) const = 0;
        virtual void start( )             = 0;
        virtual void stop ( )             = 0;
        virtual bool is_active( )   const = 0;
        virtual bool is_local( )    const = 0;

    protected:

        void mk_precall( common::connection_iface &connection,
                         const google::protobuf::MethodDescriptor *method,
                         rpc::lowlevel_unit &llu );

        void mk_postcall( common::connection_iface &connection,
                          rpc::lowlevel_unit &llu );

        void new_connection(  common::connection_iface *conn );
        void stop_connection( common::connection_iface *conn );

        void call_on_accept_failed( const VTRC_SYSTEM::error_code &err );
        void call_on_stop( );
        void call_on_start( );
    };

    typedef vtrc::shared_ptr<listener> listener_sptr;

}}

#endif
