#ifndef VTRC_CLIENT_PROTOCOL_LAYER_C_H
#define VTRC_CLIENT_PROTOCOL_LAYER_C_H

#include <stdlib.h>

#include "vtrc-common/vtrc-protocol-layer.h"

namespace vtrc {

namespace common {
    struct transport_iface;
}

namespace client {

    class protocol_layer: public common::protocol_layer {

        struct protocol_layer_c_impl;
        protocol_layer_c_impl *impl_;

        protocol_layer( const protocol_layer& other );
        protocol_layer &operator = ( const protocol_layer& other );

    public:

        protocol_layer( common::transport_iface *connection );
        ~protocol_layer( );

    public:

        void init( );
        bool ready( ) const;

    private:

        void on_data_ready( );

    };

}}


#endif // VTRCPROTOCOLLAYER_H
