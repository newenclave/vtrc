#ifndef VTRCP_ROTOCOL_LAYER_S_H
#define VTRCP_ROTOCOL_LAYER_S_H

#include <string>
#include "vtrc-common/vtrc-protocol-layer.h"

namespace vtrc {

namespace common {
    struct transport_iface;
    class call_context;
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
                          const vtrc_rpc::session_options &opts );

        ~protocol_layer_s( );

    public:

        void init( );
        const std::string &client_id( ) const;

        void close( );
        void send_and_close( const google::protobuf::MessageLite &mess );

        common::rpc_service_wrapper_sptr get_service_by_name(
                                                    const std::string &name );

    private:

        void on_data_ready( );

    };

}}



#endif // VTRCP_ROTOCOL_LAYER_H
