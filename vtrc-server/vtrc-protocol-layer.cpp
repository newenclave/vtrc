
#include <boost/asio.hpp>

#include "vtrc-protocol-layer.h"
#include "vtrc-connection-iface.h"

#include "vtrc-common/vtrc-monotonic-timer.h"
#include "vtrc-common/vtrc-data-queue.h"
#include "vtrc-common/vtrc-hasher-iface.h"
#include "vtrc-common/vtrc-hasher-impls.h"
#include "vtrc-common/vtrc-transformer-iface.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-auth.pb.h"

namespace vtrc { namespace server {

    namespace {
        static const size_t maximum_message_length = 1024 * 1024;
    }

    namespace data_queue = common::data_queue;

    struct protocol_layer::protocol_layer_impl {

        connection_iface *connection_;
        protocol_layer   *parent_;
        boost::shared_ptr<common::hasher_iface>         hasher_;
        boost::shared_ptr<common::transformer_iface>    transformer_;
        common::data_queue::queue_base_sptr             queue_;


        protocol_layer_impl( connection_iface *c )
            :connection_(c)
            ,hasher_(common::hasher::fake::create( ))
            ,transformer_(common::transformers::none::create( ))
            ,queue_(data_queue::varint::create_parser(maximum_message_length))
        {

        }

        void init( )
        {
            vtrc_auth::init_protocol hello_message;
            hello_message.set_hello_message( "Tervetuloa!" );

            hello_message.add_hash_supported( vtrc_auth::HASH_NONE );
            hello_message.add_hash_supported( vtrc_auth::HASH_SHA2_256 );

            hello_message.add_transform_supported( vtrc_auth::TRANSFORM_NONE );

            std::string data(hello_message.SerializeAsString( ));
            std::string result(queue_->pack_size( data.size( ) ));

            result.append( hasher_->get_data_hash( data.c_str( ),
                                                   data.size( ) ) );
            result.append( data.begin( ), data.end( ) );

            transformer_->transform_data( data.empty() ? NULL : &data[0],
                                          data.size( ) );

            connection_->write( data.c_str( ), data.size( ) );
        }

        void read_data( const char *data, size_t length )
        {
            if( length > 0 ) {
                std::string next_data(data, data + length);
                transformer_->revert_data( &next_data[0],
                                           next_data.size( ) );
                queue_->append( &next_data[0], next_data.size( ));
                queue_->process( );
            }
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

    void protocol_layer::read_data( const char *data, size_t length )
    {
        impl_->read_data( data, length );
    }

}}

