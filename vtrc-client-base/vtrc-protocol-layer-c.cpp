
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "vtrc-common/vtrc-data-queue.h"
#include "vtrc-common/vtrc-hasher-iface.h"
#include "vtrc-common/vtrc-transformer-iface.h"

#include "vtrc-protocol-layer-c.h"
#include "vtrc-transport-iface.h"
#include "vtrc-common/vtrc-hasher-iface.h"

#include "protocol/vtrc-auth.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace client {

    namespace gpb = google::protobuf;

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

        void pop_message( )
        {
            parent_->pop_message( );
        }

        void send_proto_message( const gpb::Message &mess ) const
        {
            std::string s(mess.SerializeAsString( ));
            connection_->write(s.c_str( ), s.size( ) );
        }

        void send_proto_message( const gpb::Message &mess,
                                 common::closure_type closure ) const
        {
            std::string s(mess.SerializeAsString( ));
            connection_->write(s.c_str( ), s.size( ), closure );
        }

        void on_server_ready( )
        {
            std::string &mess = parent_->get_data_queue( ).messages( ).front( );
            bool check = parent_->check_message( mess );
            std::cout << "Message check: " << check << "\n";
            vtrc_auth::transformer_setup transformer_proto;

            if( check ) {
                parent_->parse_message( mess, transformer_proto );
                std::cout << "transformer Message is: "
                          << transformer_proto.DebugString( ) << "\n";
            }

            vtrc_rpc_lowlevel::lowlevel_unit llu;

            llu.set_id( 100 );
            llu.mutable_call(  )->set_service( "service_name" );
            llu.mutable_call(  )->set_method( "method_name" );

            llu.mutable_info( )
                    ->set_message_type(
                        vtrc_rpc_lowlevel::message_info::MESSAGE_CALL);

            send_proto_message( llu );

            pop_message( );

        }

        void set_options( const boost::system::error_code &err )
        {
            if( !err ) {
                parent_->set_hasher_transformer(
                      common::hasher::create_by_index( vtrc_auth::HASH_CRC_64 ),
                      NULL);
            }
        }

        void on_hello_call( )
        {
            std::string &mess = parent_->get_data_queue( ).messages( ).front( );
            bool check = parent_->check_message( mess );
            std::cout << "Message check: " << check << "\n";
            vtrc_auth::init_protocol init_proto;

            if( check ) {
                parent_->parse_message( mess, init_proto );
                std::cout << "Message is: " << init_proto.DebugString( ) << "\n";
                throw std::runtime_error( "hello!" );
            }
            pop_message( );

            vtrc_auth::client_selection select;
            select.set_hash( vtrc_auth::HASH_CRC_64 );
            select.set_transform( vtrc_auth::TRANSFORM_NONE );
            select.set_ready( true );
            select.set_hello_message( "Miten menee?" );

            send_proto_message( select,
                    boost::bind( &this_type::set_options, this, _1 ) );

            stage_call_ = boost::bind( &this_type::on_server_ready, this );

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
