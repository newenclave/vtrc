#include "vtrc-protocol-layer.h"

#include "vtrc-data-queue.h"
#include "vtrc-hasher-iface.h"
#include "vtrc-hasher-impls.h"
#include "vtrc-transformer-iface.h"

#include "vtrc-transport-iface.h"

namespace vtrc { namespace common {

    namespace {
        static const size_t maximum_message_length = 1024 * 1024;
    }

    struct protocol_layer::protocol_layer_impl {

        transport_iface             *connection_;
        protocol_layer              *parent_;
        hasher_iface_sptr            hasher_;
        transformer_iface_sptr       transformer_;
        data_queue::queue_base_sptr  queue_;

        protocol_layer_impl( transport_iface *c )
            :connection_(c)
            ,hasher_(common::hasher::create_default( ))
            ,transformer_(common::transformers::none::create( ))
            ,queue_(data_queue::varint::create_parser(maximum_message_length))
        {}

        void process_data( const char *data, size_t length )
        {

        }

        void send_data( const char *data, size_t length )
        {

        }

    };

    protocol_layer::protocol_layer( transport_iface *connection )
        :impl_(new protocol_layer_impl(connection))
    {
        impl_->parent_ = this;
    }

    protocol_layer::~protocol_layer( )
    {
        delete impl_;
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
