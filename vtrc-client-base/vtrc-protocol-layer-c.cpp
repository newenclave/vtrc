
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
        {

        }

        void data_ready( )
        {
            size_t count = parent_->get_data_queue( ).messages( ).size( );
            std::cout << "recv : " << count << " messages" << std::endl;
        }

        bool ready( ) const
        {
            return true;
        }

        void on_ready( )
        {
            ;;;
        }


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

    void protocol_layer::on_data_ready( )
    {
        std::cout << "data ready\n";
        impl_->data_ready( );
    }

    bool protocol_layer::ready( ) const
    {
        return impl_->ready( );
    }

}}
