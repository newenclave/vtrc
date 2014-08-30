#ifndef VTRC_CLIENT_PROTOCOL_LAYER_C_H
#define VTRC_CLIENT_PROTOCOL_LAYER_C_H

#include <stdlib.h>

#include "vtrc-common/vtrc-protocol-layer.h"

namespace vtrc_errors {
    class container;
}

namespace vtrc {

namespace client {
    class vtrc_client;
}

namespace common {
    struct transport_iface;
}

namespace client {

    struct protocol_signals {

        virtual ~protocol_signals( ) { }

        virtual void on_init_error(const vtrc_errors::container & /*err*/,
                                   const char * /*message*/)
        {}

        virtual void on_connect( )
        {}

        virtual void on_disconnect( )
        {}

        virtual void on_ready( )
        {}

    };

    class protocol_layer_c: public common::protocol_layer {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        protocol_layer_c( const protocol_layer_c& other );
        protocol_layer_c &operator = ( const protocol_layer_c& other );

    public:

        protocol_layer_c( common::transport_iface *connection,
                          vtrc::client::vtrc_client *client,
                          protocol_signals *callbacks);
        ~protocol_layer_c( );

    public:

        void init( );
        void close( );
        const std::string &client_id( ) const;

        common::rpc_service_wrapper_sptr
                                get_service_by_name( const std::string &name );

    private:

        void on_data_ready( );
        void on_init_error( const vtrc_errors::container &error,
                            const char *message );

    };

}}


#endif // VTRCPROTOCOLLAYER_H
