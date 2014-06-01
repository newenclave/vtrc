#ifndef VTRC_ENDPOINT_IFACE_H
#define VTRC_ENDPOINT_IFACE_H

#include <string>

#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-memory.h"

namespace vtrc_rpc {
    class session_options;
}

namespace boost { namespace system {
    class error_code;
}}

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
                             void ( const boost::system::error_code &err ) );

        VTRC_DECLARE_SIGNAL( on_new_connection,
                             void ( const common::connection_iface * ) );

        VTRC_DECLARE_SIGNAL( on_stop_connection,
                             void ( const common::connection_iface * ) );


    protected:

        listener( application & app, const vtrc_rpc::session_options &opts );

    public:

        virtual ~listener( );

    public:

        application                      &get_application( );
        common::enviroment               &get_enviroment( );
        const vtrc_rpc::session_options  &get_options( ) const;
        size_t clients_count( ) const;

        vtrc::weak_ptr<listener>       weak_from_this( );
        vtrc::weak_ptr<listener const> weak_from_this( ) const;

    public:

        virtual std::string name( ) const = 0;
        virtual void start( )             = 0;
        virtual void stop ( )             = 0;

    protected:

        void new_connection(  const common::connection_iface *conn );
        void stop_connection( const common::connection_iface *conn );

        void call_on_accept_failed( const boost::system::error_code &err );
        void call_on_stop( );
        void call_on_start( );

    };

    typedef vtrc::shared_ptr<listener> listener_sptr;

    namespace listeners {
        vtrc_rpc::session_options default_options( );
    }
}}

#endif
