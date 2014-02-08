#include "vtrc-protocol-layer.h"

#include "vtrc-data-queue.h"
#include "vtrc-hasher-iface.h"
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

        // --------------- sett ----------------- //

        hasher_iface &get_hasher( )
        {
            return *hasher_;
        }

        const hasher_iface &get_hasher( ) const
        {
            return *hasher_;
        }

        data_queue::queue_base &get_data_queue( )
        {
            return *queue_;
        }

        const data_queue::queue_base &get_data_queue( ) const
        {
            return *queue_;
        }

        transformer_iface &get_transformer( )
        {
            return *transformer_;
        }

        const transformer_iface &get_transformer( ) const
        {
            return *transformer_;
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

    hasher_iface &protocol_layer::get_hasher( )
    {
        return impl_->get_hasher( );
    }

    const hasher_iface &protocol_layer::get_hasher( ) const
    {
        return impl_->get_hasher( );
    }

    data_queue::queue_base &protocol_layer::get_data_queue( )
    {
        return impl_->get_data_queue( );
    }

    const data_queue::queue_base &protocol_layer::get_data_queue( ) const
    {
        return impl_->get_data_queue( );
    }

    transformer_iface &protocol_layer::get_transformer( )
    {
        return impl_->get_transformer( );
    }

    const transformer_iface &protocol_layer::get_transformer( ) const
    {
        return impl_->get_transformer( );
    }

}}
