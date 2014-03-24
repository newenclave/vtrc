
#include <boost/asio.hpp>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "vtrc-bind.h"
#include "vtrc-function.h"

#include "vtrc-common/vtrc-data-queue.h"
#include "vtrc-common/vtrc-hash-iface.h"
#include "vtrc-common/vtrc-transformer-iface.h"
#include "vtrc-common/vtrc-call-context.h"

#include "vtrc-protocol-layer-c.h"
#include "vtrc-transport-iface.h"

#include "vtrc-client.h"

#include "protocol/vtrc-auth.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace client {

    namespace gpb = google::protobuf;

    namespace {
        typedef vtrc::shared_ptr <
            vtrc_rpc_lowlevel::lowlevel_unit
        > lowlevel_unit_sptr;

        typedef vtrc::shared_ptr<gpb::Service> service_str;

    }

    struct protocol_layer_c::impl {

        typedef impl this_type;
        typedef protocol_layer_c parent_type;

        typedef vtrc::function<void (void)> stage_funcion_type;

        common::transport_iface     *connection_;
        protocol_layer_c            *parent_;
        stage_funcion_type           stage_call_;
        vtrc_client                 *client_;
        vtrc::shared_ptr<int>        track_;

        impl( common::transport_iface *c, vtrc_client *client )
            :connection_(c)
            ,client_(client)
            ,track_(vtrc::make_shared<int>(1))
        {
            stage_call_ = vtrc::bind( &this_type::on_hello_call, this );
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

        void process_event_impl( vtrc_client_wptr client,
                                 lowlevel_unit_sptr llu,
                                 service_str serv,
                                 const gpb::MethodDescriptor *meth )
        {
            vtrc_client_sptr c(client.lock( ));
            if( !c ) return;

            protocol_layer::context_holder ch( parent_, llu.get( ) );

            const vtrc_rpc_lowlevel::options &opts(
                        parent_->get_method_options( meth ) );

            ch.ctx_->set_call_options( opts );
        }

        void process_event( lowlevel_unit_sptr &llu )
        {
            const std::string serv_name( llu->call( ).service( ) );
            service_str serv = client_->get_handler( serv_name );

            if( serv ) {
                const std::string meth_name( llu->call( ).method( ) );
                const gpb::MethodDescriptor *meth =
                        serv->GetDescriptor( )->FindMethodByName( meth_name );
                if( meth ) {

                    const vtrc_rpc_lowlevel::options &opts(
                                parent_->get_method_options( meth ) );

                    client_->get_io_service( ).post(
                        vtrc::bind( &this_type::process_event_impl, this,
                                 client_->weak_from_this( ),
                                 llu, serv, meth));
                }
            }
        }

        void process_callback( lowlevel_unit_sptr &llu )
        {

        }

        void process_service( lowlevel_unit_sptr &llu )
        {

        }

        void process_call( lowlevel_unit_sptr &llu )
        {
            parent_->push_rpc_message( llu->id( ), llu );
        }

        void process_invalid( lowlevel_unit_sptr &llu )
        {

        }

        void on_system_error(const boost::system::error_code &err,
                             const std::string &add)
        {
            vtrc::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit>
                            llu( new  vtrc_rpc_lowlevel::lowlevel_unit );

            vtrc_errors::error_container *err_cont = llu->mutable_error( );

            err_cont->set_code(err.value( ));
            err_cont->set_category(vtrc_errors::CATEGORY_SYSTEM);
            err_cont->set_fatal( true );
            err_cont->set_additional( add );

            parent_->push_rpc_message_all( llu );
        }

        void on_rpc_process( )
        {

            while( !parent_->message_queue( ).empty( ) ) {
                std::string &mess
                        (parent_->message_queue( ).front( ));

                bool check = parent_->check_message( mess );

                vtrc::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit>
                                llu( new  vtrc_rpc_lowlevel::lowlevel_unit );

                if( !check ) {

                    vtrc_errors::error_container *err = llu->mutable_error( );
                    err->set_code(vtrc_errors::ERR_PROTOCOL);
                    err->set_additional("Bad message was received");
                    err->set_fatal( true );

                    parent_->push_rpc_message_all( llu );
                    connection_->close( );

                    return;
                }

                parent_->parse_message( mess, *llu );

                switch( llu->info( ).message_type( ) ) {

                case vtrc_rpc_lowlevel::message_info::MESSAGE_EVENT:
                    process_event( llu );
                    break;
                case vtrc_rpc_lowlevel::message_info::MESSAGE_CALLBACK:
                    process_callback( llu );
                    break;
                case vtrc_rpc_lowlevel::message_info::MESSAGE_SERVICE:
                    process_service( llu );
                    break;
                case vtrc_rpc_lowlevel::message_info::MESSAGE_CALL:
                    process_call( llu );
                    break;
                default:
                    process_invalid( llu );
                }

                pop_message( );
            }
        }

        void on_server_ready( )
        {
            std::string &mess = parent_->message_queue( ).front( );
            bool check = parent_->check_message( mess );
            vtrc_auth::transformer_setup transformer_proto;

            if( check ) {
                parent_->parse_message( mess, transformer_proto );
            }

            pop_message( );
            stage_call_ = vtrc::bind( &this_type::on_rpc_process, this );
        }

        void set_options( const boost::system::error_code &err )
        {
            if( !err ) {
                parent_->change_hash_maker(
                   common::hash::create_by_index( vtrc_auth::HASH_CRC_64 ));
            }
        }

        void on_hello_call( )
        {
            std::string &mess = parent_->message_queue( ).front( );
            bool check = parent_->check_message( mess );

            vtrc_auth::init_protocol init_proto;

            if( check ) {
                parent_->parse_message( mess, init_proto );
                std::cout << "Message is: "
                          << init_proto.DebugString( ) << "\n";
            }

            pop_message( );

            vtrc_auth::client_selection select;
            select.set_hash( vtrc_auth::HASH_CRC_64 );
            select.set_transform( vtrc_auth::TRANSFORM_NONE );
            select.set_ready( true );
            select.set_hello_message( "Miten menee?" );

            parent_->change_hash_checker(
               common::hash::create_by_index( vtrc_auth::HASH_CRC_64 ));

            send_proto_message( select,
                    vtrc::bind( &this_type::set_options, this, _1 ) );

            stage_call_ = vtrc::bind( &this_type::on_server_ready, this );

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

    protocol_layer_c::protocol_layer_c( common::transport_iface *connection,
                                        vtrc::client::vtrc_client *client )
        :common::protocol_layer(connection)
        ,impl_(new impl(connection, client))
    {
        impl_->parent_ = this;
    }

    protocol_layer_c::~protocol_layer_c( )
    {
        delete impl_;
    }

    void protocol_layer_c::on_read_error(const boost::system::error_code &err)
    {
        impl_->on_system_error( err, "Transport read error." );
    }

    void protocol_layer_c::on_write_error(const boost::system::error_code &err)
    {
        impl_->on_system_error( err, "Transport write error." );
    }

    void protocol_layer_c::init( )
    {
        impl_->init( );
    }

    void protocol_layer_c::on_data_ready( )
    {
        impl_->data_ready( );
    }

    bool protocol_layer_c::ready( ) const
    {
        return impl_->ready( );
    }


}}
