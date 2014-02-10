
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "vtrc-common/vtrc-data-queue.h"
#include "vtrc-common/vtrc-hasher-iface.h"
#include "vtrc-common/vtrc-transformer-iface.h"

#include "vtrc-protocol-layer-c.h"
#include "vtrc-transport-iface.h"

namespace vtrc { namespace client {

    struct protocol_layer::protocol_layer_c_impl {

        typedef protocol_layer_c_impl this_type;

        typedef boost::function<void (void)> stage_funcion_type;

        common::transport_iface *connection_;
        protocol_layer          *parent_;
        stage_funcion_type       stage_call_;

        protocol_layer_c_impl( common::transport_iface *c )
            :connection_(c)
        {
            stage_call_ = boost::bind( &this_type::on_hello_call, this );
        }

        void init( )
        {

        }

        void on_hello_call( )
        {

        }

        void data_ready( )
        {
            stage_call_( );
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
