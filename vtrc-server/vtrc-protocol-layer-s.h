#ifndef VTRCP_ROTOCOL_LAYER_S_H
#define VTRCP_ROTOCOL_LAYER_S_H

#include "vtrc-common/vtrc-protocol-layer.h"

namespace vtrc {

namespace common {
    struct transport_iface;
}

namespace server {

    class application;

    class protocol_layer: public common::protocol_layer {

        struct protocol_layer_s_impl;
        protocol_layer_s_impl *impl_;

        protocol_layer( const protocol_layer& other );
        protocol_layer operator = ( const protocol_layer& other );

    public:

        protocol_layer( application &app,
                        common::transport_iface *connection);
        ~protocol_layer( );

    public:

        void init( );
        bool ready( ) const;
    private:

        void on_data_ready( );

    };

}}



#endif // VTRCP_ROTOCOL_LAYER_H
