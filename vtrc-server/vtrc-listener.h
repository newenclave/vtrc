#ifndef VTRC_ENDPOINT_IFACE_H
#define VTRC_ENDPOINT_IFACE_H

#include <string>

#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-memory.h"

namespace vtrc {

    namespace common {
        class  enviroment;
        struct connection_iface;
    }

namespace server {

    class application;

    struct listener_options {
        unsigned maximum_active_calls;
        unsigned maximum_message_length;
        unsigned maximum_total_calls;
        unsigned read_buffer_size;
    };

    class listener: public vtrc::enable_shared_from_this<listener> {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        VTRC_DECLARE_SIGNAL( on_start,          void( ) );
        VTRC_DECLARE_SIGNAL( on_stop,           void( ) );
        VTRC_DECLARE_SIGNAL( on_new_connection,
                             void( const common::connection_iface * ) );

        VTRC_DECLARE_SIGNAL( on_stop_connection,
                             void( const common::connection_iface * ) );


    public:

        listener( application & app, const listener_options &opts );
        virtual ~listener( );

    public:

        application             &get_application( );
        common::enviroment      &get_enviroment( );
        const listener_options  &get_options( ) const;
        size_t clients_count( ) const;

        vtrc::weak_ptr<listener> weak_from_this( )
        {
            return vtrc::weak_ptr<listener>( shared_from_this( ) );
        }

        vtrc::weak_ptr<listener const> weak_from_this( ) const
        {
            return vtrc::weak_ptr<listener const>( shared_from_this( ));
        }

    public:

        virtual std::string string( ) const = 0;

        virtual void start( ) = 0;
        virtual void stop ( ) = 0;


    protected:

        void new_connection( const common::connection_iface *conn );
        void stop_connection( const common::connection_iface *conn );

    };

    typedef vtrc::shared_ptr<listener> listener_sptr;

    namespace listeners {
        listener_options default_options( );
    }
}}

#endif