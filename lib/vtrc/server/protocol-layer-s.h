#ifndef VTRCP_ROTOCOL_LAYER_S_H
#define VTRCP_ROTOCOL_LAYER_S_H

#include <string>
#include "vtrc/common/protocol-layer.h"
#include "vtrc/common/closure.h"
#include "vtrc/common/lowlevel-protocol-iface.h"

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
                          common::connection_iface *connection,
                          const rpc::session_options &opts );

        ~protocol_layer_s( );

    public:

        typedef common::lowlevel::protocol_factory_type lowlevel_factory_type;

        void init( );
        void init_success( common::system_closure_type clos );
        const std::string &client_id( ) const;

        void close( );

        void drop_service( const std::string &name );
        void drop_all_services(  );

        void assign_lowlevel_factory( lowlevel_factory_type factory );


        common::rpc_service_wrapper_sptr get_service_by_name(
                                                    const std::string &name );

    };

}}



#endif // VTRCP_ROTOCOL_LAYER_H
