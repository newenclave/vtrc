
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

#include "vtrc-common/vtrc-rpc-service-wrapper.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-auth.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace server {

    namespace gpb = google::protobuf;

    namespace {
        enum init_stage_enum {
             stage_begin             = 1
            ,stage_client_select     = 2
            ,stage_client_ready      = 3
        };

        typedef std::map <
            std::string,
            common::rpc_service_wrapper_sptr
        > service_map;

    }

    namespace data_queue = common::data_queue;

    struct protocol_layer::protocol_layer_s_impl {

        typedef protocol_layer_s_impl this_type;

        application             &app_;
        common::transport_iface *connection_;
        protocol_layer          *parent_;
        bool                     ready_;

        service_map              services_;
        boost::shared_mutex      services_lock_;

        typedef boost::function<void (void)> stage_function_type;
        stage_function_type  stage_function_;

        protocol_layer_s_impl( application &a, common::transport_iface *c )
            :app_(a)
            ,connection_(c)
            ,ready_(false)
        {
            stage_function_ =
                    boost::bind( &this_type::on_client_selection, this );
        }

        void pop_message( )
        {
            parent_->pop_message( );
        }

        void on_timer( boost::system::error_code const &error )
        {

        }

        bool check_message_hash( const std::string &mess )
        {
            return parent_->check_message( mess );
        }

        void write( const char *data, size_t length )
        {
            connection_->write( data, length );
        }

        void write( const char *data, size_t length ) const
        {
            connection_->write( data, length );
        }

        void send_proto_message( const gpb::Message &mess ) const
        {
            std::string s(mess.SerializeAsString( ));
            write( s.c_str( ), s.size( ) );
        }

        void on_client_selection( )
        {
            std::string &mess(parent_->get_data_queue( ).messages( ).front( ));
            bool check = check_message_hash(mess);
            if( !check ) {
                std::cout << "bad hash\n";
                connection_->close( );
                return;
            }
            vtrc_auth::client_selection cs;
            parse_message( mess, cs );

            std::cout << cs.DebugString( ) << "\n";

            pop_message( );

            parent_->set_hasher_transformer(
                        common::hasher::create_by_index( cs.hash( ) ),
                        NULL);

            vtrc_auth::transformer_setup ts;

            ts.set_ready( true );
            ts.set_opt_message( "good" );

            stage_function_ =
                    boost::bind( &this_type::on_rcp_call_ready, this );

            send_proto_message( ts );

        }

        void get_pop_message( gpb::Message &capsule )
        {
            std::string &mess(parent_->get_data_queue( ).messages( ).front( ));
            bool check = check_message_hash(mess);
            if( !check ) {
                std::runtime_error("bad hash");
                connection_->close( );
                return;
            }
            parse_message( mess, capsule );
            pop_message( );
        }

        void on_rcp_call_ready( )
        {
            while( !parent_->get_data_queue( ).messages( ).empty( ) ) {
                vtrc_rpc_lowlevel::lowlevel_unit llu;
                get_pop_message( llu );

                try {
                    std::cout << llu.DebugString( ) << "\n";
                } catch( const std::exception &ex ) {
                    std::cout << "Error rpc: " << ex.what( ) << "\n";
                }
            }

            //connection_->close( );
        }

        void parse_message( const std::string &block, gpb::Message &mess )
        {
            parent_->parse_message( block, mess );
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

        bool ready( ) const
        {
            return true;
        }

    };

    protocol_layer::protocol_layer( application &a,
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

    void protocol_layer::on_data_ready( )
    {
        impl_->data_ready( );
    }

    bool protocol_layer::ready( ) const
    {
        return impl_->ready( );
    }

}}

