
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

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

    namespace gpb = google::protobuf;

    namespace {
        static const size_t maximum_message_length = 1024 * 1024;
        enum init_stage_enum {
             stage_begin             = 1
            ,stage_client_select     = 2
            ,stage_client_ready      = 3
        };
    }

    namespace data_queue = common::data_queue;

    struct protocol_layer::protocol_layer_impl {

        typedef protocol_layer_impl this_type;

        connection_iface *connection_;
        protocol_layer   *parent_;
        boost::shared_ptr<common::hasher_iface>      hasher_;
        boost::shared_ptr<common::transformer_iface> transformer_;
        common::data_queue::queue_base_sptr          queue_;
        init_stage_enum                              init_stages_;

        typedef boost::function<void (void)> stage_function_type;
        stage_function_type  stage_function_;

        protocol_layer_impl( connection_iface *c )
            :connection_(c)
            ,hasher_(common::hasher::create_default( ))
            ,transformer_(common::transformers::none::create( ))
            ,queue_(data_queue::varint::create_parser(maximum_message_length))
            ,init_stages_(stage_begin)
        {
            stage_function_ =
                    boost::bind( &this_type::on_client_selection, this );
        }

        void on_timer( boost::system::error_code const &error )
        {

        }

        void on_client_selection( )
        {

        }

        void on_ready( )
        {

        }

        void on_rcp_call_ready( )
        {

        }

        bool check_message_hash( const std::string &mess )
        {
            const size_t hash_length = hasher_->hash_size( );
            const size_t diff_len = mess.size( ) - hash_length;

            bool result = false;

            if( mess.size( ) >= hash_length ) {
                result = hasher_->
                        check_data_hash( mess.c_str( ) + hash_length, diff_len,
                                         mess.c_str( ) );
            }
            return result;
        }

        void parse_message( const std::string &block, gpb::Message &mess )
        {
            const size_t hash_length = hasher_->hash_size( );
            const size_t diff_len = block.size( ) - hash_length;
            mess.ParseFromArray( block.c_str( ) + hash_length, diff_len );
        }

        void init( )
        {
            vtrc_auth::init_protocol hello_mess;
            hello_mess.set_hello_message( "Tervetuloa!" );

            hello_mess.add_hash_supported( vtrc_auth::HASH_NONE );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_16 );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_32 );
            hello_mess.add_hash_supported( vtrc_auth::HASH_SHA2_256 );

            hello_mess.add_transform_supported( vtrc_auth::TRANSFORM_NONE );
            hello_mess.add_transform_supported( vtrc_auth::TRANSFORM_ERSEEFOR );

            std::string data(hello_mess.SerializeAsString( ));
            std::string result(queue_->pack_size( data.size( ) ));

            result.append( hasher_->get_data_hash( data.c_str( ),
                                                   data.size( ) ) );
            result.append( data.begin( ), data.end( ) );

            transformer_->transform_data( data.empty( ) ? NULL : &data[0],
                                          data.size( ) );

            connection_->write( data.c_str( ), data.size( ) );
        }

        void process_data( const char *data, size_t length )
        {
            if( length > 0 ) {
                std::string next_data(data, data + length);
                transformer_->revert_data( &next_data[0],
                                           next_data.size( ) );
                queue_->append( &next_data[0], next_data.size( ));
                queue_->process( );
                stage_function_( );
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

    void protocol_layer::process_data( const char *data, size_t length )
    {
        impl_->process_data( data, length );
    }

}}

