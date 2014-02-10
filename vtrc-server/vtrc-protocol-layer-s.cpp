
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "vtrc-protocol-layer-s.h"

#include "vtrc-monotonic-timer.h"
#include "vtrc-data-queue.h"
#include "vtrc-hasher-iface.h"
#include "vtrc-transformer-iface.h"

#include "vtrc-transport-iface.h"

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

    struct protocol_layer::protocol_layer_s_impl {

        typedef protocol_layer_s_impl this_type;

        application             &app_;
        common::transport_iface *connection_;
        protocol_layer          *parent_;

        typedef boost::function<void (void)> stage_function_type;
        stage_function_type  stage_function_;

        boost::mutex  write_locker_; // use strand!

        protocol_layer_s_impl( application       &a,
                               common::transport_iface *c )
            :app_(a)
            ,connection_(c)
        {
            stage_function_ =
                    boost::bind( &this_type::on_client_selection, this );
        }

        void on_timer( boost::system::error_code const &error )
        {

        }

        bool check_message_hash( const std::string &mess )
        {
            const size_t hash_length = parent_->get_hasher( ).hash_size( );
            const size_t diff_len    = mess.size( ) - hash_length;

            bool result = false;

            if( mess.size( ) >= hash_length ) {
                result = parent_->get_hasher( ).
                         check_data_hash( mess.c_str( ) + hash_length,
                                         diff_len,
                                         mess.c_str( ) );
            }
            return result;
        }

        void write( const char *data, size_t length )
        {
            parent_->send_data( data, length );
        }

        void on_client_selection( )
        {
            bool check = check_message_hash(
                        parent_->get_data_queue( ).messages( ).front( ) );
            if( !check ) {
                std::cout << "bad hash\n";
                connection_->close( );
            }
        }

        void on_ready( )
        {

        }

        void on_rcp_call_ready( )
        {

        }

        void parse_message( const std::string &block, gpb::Message &mess )
        {
            const size_t hash_length = parent_->get_hasher( ).hash_size( );
            const size_t diff_len    = block.size( ) - hash_length;
            mess.ParseFromArray( block.c_str( ) + hash_length, diff_len );
        }

        std::string first_message( )
        {
            vtrc_auth::init_protocol hello_mess;
            hello_mess.set_hello_message( "Tervetuloa!" );

            hello_mess.add_hash_supported( vtrc_auth::HASH_NONE     );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_16   );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_32   );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_64   );
            hello_mess.add_hash_supported( vtrc_auth::HASH_SHA2_256 );

            hello_mess.add_transform_supported( vtrc_auth::TRANSFORM_NONE );
            hello_mess.add_transform_supported( vtrc_auth::TRANSFORM_ERSEEFOR );
            return hello_mess.SerializeAsString( );
        }

        void init( )
        {
            static const std::string data(first_message( ));
            write(data.c_str( ), data.size( ));
        }

        void data_ready( )
        {
            stage_function_( );
        }

    };

    protocol_layer::protocol_layer( application       &a,
                                    common::transport_iface *connection )
        :common::protocol_layer(connection)
        ,impl_(new protocol_layer_s_impl(a, connection))
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

    void protocol_layer::data_ready( )
    {
        impl_->data_ready( );
    }

}}

