#ifndef VTRC_CLIENT_PROTOCOL_LAYER_C_H
#define VTRC_CLIENT_PROTOCOL_LAYER_C_H

#include <stdlib.h>

#include "vtrc/common/protocol-layer.h"
#include "vtrc/common/lowlevel-protocol-iface.h"

#include "vtrc-function.h"

namespace vtrc {

namespace rpc { namespace errors {
    class container;
}}

namespace client {
    class base;
}

namespace common {
    struct transport_iface;
}

namespace client {

    struct protocol_signals {

        virtual ~protocol_signals( ) { }

        virtual void on_init_error(const rpc::errors::container & /*err*/,
                                   const char * /*message*/)
        { }

        virtual void on_connect( )
        { }

        virtual void on_disconnect( )
        { }

        virtual void on_ready( bool )
        { }

    };

    class protocol_layer_c: public common::protocol_layer {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        protocol_layer_c( const protocol_layer_c& other );
        protocol_layer_c &operator = ( const protocol_layer_c& other );

    public:

        protocol_layer_c( common::connection_iface *connection,
                          vtrc::client::base *client,
                          protocol_signals *callbacks);
        ~protocol_layer_c( );

    public:

        typedef common::lowlevel::protocol_factory_type lowlevel_factory_type;

        void init( );
        void close( );
        const std::string &client_id( ) const;

        common::rpc_service_wrapper_sptr
                                get_service_by_name( const std::string &name );
        void drop_service( const std::string &name );
        void drop_all_services(  );

    private:

        void on_init_error( const rpc::errors::container &error,
                            const char *message );

    };

}}


#endif // VTRCPROTOCOLLAYER_H
