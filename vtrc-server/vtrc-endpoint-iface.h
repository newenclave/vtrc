#ifndef VTRC_ENDPOINT_IFACE_H
#define VTRC_ENDPOINT_IFACE_H

#include <string>

namespace vtrc { namespace server {

    struct application_iface;

    struct endpoint_iface {

        virtual ~endpoint_iface( ) { }
        virtual application_iface &application( ) = 0;

        virtual std::string string( ) const = 0;

        virtual void start( ) = 0;
        virtual void stop ( ) = 0;
    };
}}

#endif
