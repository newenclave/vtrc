#ifndef VTRC_CLIENT_PROTOCOL_LAYER_C_H
#define VTRC_CLIENT_PROTOCOL_LAYER_C_H

#include <stdlib.h>

#include "vtrc-common/vtrc-protocol-layer.h"

namespace vtrc {

namespace client {
    class vtrc_client;
}

namespace common {
    struct transport_iface;
}

namespace client {

    class protocol_layer_c: public common::protocol_layer {

        struct impl;
        impl *impl_;

        protocol_layer_c( const protocol_layer_c& other );
        protocol_layer_c &operator = ( const protocol_layer_c& other );

    public:

        protocol_layer_c( common::transport_iface *connection,
                          vtrc::client::vtrc_client *client );
        ~protocol_layer_c( );

    public:

        void init( );
        bool ready( ) const;

    private:

        void on_data_ready( );

    };

}}


#endif // VTRCPROTOCOLLAYER_H
