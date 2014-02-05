
#include <boost/asio.hpp>

#include "vtrc-protocol-layer.h"
#include "vtrc-connection-iface.h"

#include "vtrc-common/vtrc-monotonic-timer.h"

namespace vtrc { namespace server {

    struct protocol_layer::protocol_layer_impl {

        connection_iface *connection_;
        protocol_layer   *parent_;

        protocol_layer_impl( connection_iface *c )
            :connection_(c)
        {

        }

        void init( )
        {

        }

    };

    protocol_layer::protocol_layer( connection_iface *connection )
        :impl_(new protocol_layer_impl(connection))
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

}}

