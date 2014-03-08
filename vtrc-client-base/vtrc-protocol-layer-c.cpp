
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

    struct protocol_layer::impl {

        typedef impl this_type;

        typedef boost::function<void (void)> stage_funcion_type;

        common::transport_iface *connection_;
        protocol_layer          *parent_;
        stage_funcion_type       stage_call_;

        impl( common::transport_iface *c )
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

        void on_rpc_process( )
        {
            while( !parent_->get_data_queue( ).messages( ).empty( ) ) {
                std::string &mess
                        (parent_->get_data_queue( ).messages( ).front( ));
                bool check = parent_->check_message( mess );

                boost::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit>
                        llu( new  vtrc_rpc_lowlevel::lowlevel_unit );
                parent_->parse_message( mess, *llu );
                std::cout << "push " << llu->id( ) << "\n";
                parent_->push_rpc_message( llu->id( ), llu );
                pop_message( );
            }
        }

        void on_server_ready( )
        {
            std::string &mess = parent_->get_data_queue( ).messages( ).front( );
            bool check = parent_->check_message( mess );
            vtrc_auth::transformer_setup transformer_proto;

            if( check ) {
                parent_->parse_message( mess, transformer_proto );
            }

            pop_message( );
            stage_call_ = boost::bind( &this_type::on_rpc_process, this );
        }

        void set_options( const boost::system::error_code &err )
        {
            if( !err ) {
                parent_->change_sign_maker(
                   common::hasher::create_by_index( vtrc_auth::HASH_CRC_64 ));
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
            }

            pop_message( );

            vtrc_auth::client_selection select;
            select.set_hash( vtrc_auth::HASH_CRC_64 );
            select.set_transform( vtrc_auth::TRANSFORM_NONE );
            select.set_ready( true );
            select.set_hello_message( "Miten menee?" );

            parent_->change_sign_checker(
               common::hasher::create_by_index( vtrc_auth::HASH_CRC_64 ));

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
        ,impl_(new impl(connection))
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
