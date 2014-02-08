
#include "vtrc-common/vtrc-data-queue.h"
#include "vtrc-common/vtrc-hasher-iface.h"
#include "vtrc-common/vtrc-transformer-iface.h"

#include "vtrc-protocol-layer-c.h"
#include "vtrc-transport-iface.h"

namespace vtrc { namespace client {

    struct protocol_layer::protocol_layer_c_impl {

        common::transport_iface *connection_;
        protocol_layer          *parent_;

        protocol_layer_c_impl( common::transport_iface *c )
            :connection_(c)
        {}

        void init( )
        {}

        void process_data( const char *data, size_t length )
        {}

        void send_data( const char *data, size_t length )
        {}

    };

    protocol_layer::protocol_layer( common::transport_iface *connection )
        :common::protocol_layer(connection)
        ,impl_(new protocol_layer_c_impl(connection))
    {
        impl_->parent_ = this;
    }

    protocol_layer::~protocol_layer( )
    {
        delete impl_;
    }

    void protocol_layer::init( )
    {
        impl_->init( );
    }

    void protocol_layer::process_data( const char *data, size_t length )
    {
        impl_->process_data( data, length );
    }

    void protocol_layer::send_data( const char *data, size_t length )
    {
        impl_->send_data( data, length );
    }

}}
