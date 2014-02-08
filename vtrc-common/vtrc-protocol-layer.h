#ifndef VTRC_PROTOCOL_LAYER_H
#define VTRC_PROTOCOL_LAYER_H

#include <stdlib.h>

namespace vtrc { namespace common {

    struct transport_iface;

    class protocol_layer {

        struct protocol_layer_impl;
        protocol_layer_impl *impl_;

        protocol_layer( const protocol_layer& other );
        protocol_layer &operator = ( const protocol_layer& other );

    public:

        protocol_layer( transport_iface *connection );
        virtual ~protocol_layer( );

    public:

        virtual void init( ) = 0;
        void process_data( const char *data, size_t length );
        void send_data( const char *data, size_t length );

    };
}}

#endif // VTRCPROTOCOLLAYER_H
