#ifndef VTRCP_ROTOCOL_LAYER_S_H
#define VTRCP_ROTOCOL_LAYER_S_H

#include "vtrc-common/vtrc-protocol-layer.h"

namespace vtrc {

namespace common {
    struct transport_iface;
}

namespace server {

    class application;

    class protocol_layer_s: public common::protocol_layer {

        struct impl;
        impl *impl_;

        protocol_layer_s( const protocol_layer_s& other );
        protocol_layer_s operator = ( const protocol_layer_s& other );

    public:

        protocol_layer_s( application &app,
                          common::transport_iface *connection,
                          unsigned maximum_active_calls,
                          size_t maximum_message_length);

        ~protocol_layer_s( );

    public:

        void init( );
        bool ready( ) const;

        common::rpc_service_wrapper_sptr get_service_by_name(
                                                    const std::string &name );

    private:

        void on_data_ready( );

    };

}}



#endif // VTRCP_ROTOCOL_LAYER_H
