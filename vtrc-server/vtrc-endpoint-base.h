#ifndef VTRC_ENDPOINT_IFACE_H
#define VTRC_ENDPOINT_IFACE_H

#include <string>

//#include "vtrc-common/vtrc-signal-declaration.h"

namespace vtrc {

    namespace common {
        class enviroment;
    }

namespace server {

    class application;

    struct endpoint_options {
        unsigned maximum_active_calls;
        unsigned maximum_message_length;
        unsigned maximum_total_calls;
        unsigned read_buffer_size;
    };

    class endpoint_base {

        struct  impl;
        impl   *impl_;

    public:

        endpoint_base( application & app, const endpoint_options &opts);
        virtual ~endpoint_base( );

    public:

        application             &get_application( );
        common::enviroment      &get_enviroment( );
        const endpoint_options  &get_options( ) const;

    public:

        virtual std::string string( ) const = 0;

        virtual void start( ) = 0;
        virtual void stop ( ) = 0;

        virtual size_t clients_count( ) const = 0;

    };

    namespace endpoints {
        endpoint_options default_options( );
    }
}}

#endif
