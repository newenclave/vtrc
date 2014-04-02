#ifndef VTRC_ENDPOINT_IFACE_H
#define VTRC_ENDPOINT_IFACE_H

#include <string>

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

    struct endpoint_iface {

        virtual ~endpoint_iface( ) { }
        virtual application         &get_application( ) = 0;
        virtual common::enviroment  &get_enviroment( ) = 0;

        virtual std::string string( ) const = 0;

        virtual const endpoint_options &get_options( ) const = 0;

//        virtual void lock_call( ) = 0;
//        virtual void release_call( ) = 0;
//        virtual unsigned current_calls( ) = 0;

        virtual void start( ) = 0;
        virtual void stop ( ) = 0;
    };
}}

#endif
