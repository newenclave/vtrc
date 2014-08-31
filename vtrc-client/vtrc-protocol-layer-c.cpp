
#include <algorithm>

#include "boost/asio.hpp"
#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"

#include "vtrc-bind.h"
#include "vtrc-function.h"
#include "vtrc-chrono.h"

#include "vtrc-common/vtrc-data-queue.h"
#include "vtrc-common/vtrc-hash-iface.h"
#include "vtrc-common/vtrc-transformer-iface.h"
#include "vtrc-common/vtrc-call-context.h"
#include "vtrc-common/vtrc-rpc-controller.h"
#include "vtrc-common/vtrc-random-device.h"
#include "vtrc-common/vtrc-transport-iface.h"

#include "vtrc-protocol-layer-c.h"

#include "vtrc-client.h"

#include "protocol/vtrc-auth.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace client {

    namespace gpb = google::protobuf;

    namespace {
        typedef vtrc::shared_ptr <
            vtrc_rpc::lowlevel_unit
        > lowlevel_unit_sptr;

        typedef vtrc::shared_ptr<gpb::Service> service_str;

        enum protocol_stage {
             STAGE_HELLO = 1
            ,STAGE_SETUP = 2
            ,STAGE_READY = 3
            ,STAGE_RPC   = 4
        };

    }

    struct protocol_layer_c::impl {

        typedef impl this_type;
        typedef protocol_layer_c parent_type;

        typedef vtrc::function<void (void)> stage_funcion_type;

        common::transport_iface     *connection_;
        protocol_layer_c            *parent_;
        stage_funcion_type           stage_call_;
        vtrc_client                 *client_;
        protocol_signals            *callbacks_;
        protocol_stage               stage_;

        impl( common::transport_iface *c, vtrc_client *client,
              protocol_signals *cb )
            :connection_(c)
            ,client_(client)
            ,callbacks_(cb)
            ,stage_(STAGE_HELLO)
        {
            stage_call_ = vtrc::bind( &this_type::on_hello_call, this );
        }

        void change_stage( protocol_stage stage )
        {
            stage_ = stage;
            switch ( stage_ ) {
            case STAGE_HELLO:
                stage_call_ = vtrc::bind( &this_type::on_hello_call, this );
                break;
            case STAGE_SETUP:
                stage_call_ = vtrc::bind( &this_type::on_trans_setup, this );
                break;
            case STAGE_READY:
                stage_call_ = vtrc::bind( &this_type::on_server_ready, this );
                break;
            case STAGE_RPC:
                stage_call_ = vtrc::bind( &this_type::on_rpc_process, this );
                break;
            }
        }

        void init( )
        {

        }

        void check_disconnect_stage( )
        {
            switch ( stage_ ) {
            case STAGE_HELLO:
                parent_->on_init_error(
                            create_error( vtrc_errors::ERR_BUSY, "" ),
                            "Server in not ready");
                break;
            case STAGE_SETUP:
                parent_->on_init_error(
                            create_error( vtrc_errors::ERR_INVALID_VALUE, "" ),
                            "Bad setup info.");
                break;
            case STAGE_READY:
                parent_->on_init_error(
                            create_error( vtrc_errors::ERR_INTERNAL, "" ),
                            "Bad session key.");
                break;
            case STAGE_RPC:
            default:
                break;
            }
        }

        vtrc_errors::container create_error( unsigned code,
                                             const std::string &add )
        {
            vtrc_errors::container cont;
            cont.set_code( code );
            cont.set_category( vtrc_errors::CATEGORY_INTERNAL );
            cont.set_additional( add );
            return cont;
        }

        const std::string &client_id( ) const
        {
            return client_->get_session_id( );
        }

        typedef common::rpc_service_wrapper_sptr wrapper_sptr;

        wrapper_sptr get_service_by_name( const std::string &name )
        {
            return wrapper_sptr (new common::rpc_service_wrapper(
                                 client_->get_rpc_handler( name )));
        }

        void send_proto_message( const gpb::MessageLite &mess ) const
        {
            std::string s(mess.SerializeAsString( ));
            connection_->write(s.c_str( ), s.size( ) );
        }

        void send_proto_message( const gpb::MessageLite &mess,
                                 common::system_closure_type closure,
                                 bool on_send ) const
        {
            std::string s(mess.SerializeAsString( ));
            connection_->write(s.c_str( ), s.size( ), closure, on_send );
        }

        void process_event_impl( vtrc_client_wptr client,
                                 lowlevel_unit_sptr llu)
        {
            vtrc_client_sptr c(client.lock( ));
            if( !c ) return;
            parent_->make_local_call( llu );
        }

        void process_event( lowlevel_unit_sptr &llu )
        {
            if ( llu->has_id( ) ) {
                client_->get_rpc_service( ).post(
                    vtrc::bind( &this_type::process_event_impl, this,
                             client_->weak_from_this( ), llu));
            } else {
                parent_->push_rpc_message_all( llu );
            }
        }

        void process_service( lowlevel_unit_sptr &llu )
        {

        }

        void process_internal( lowlevel_unit_sptr &llu )
        {

        }

        void process_call( lowlevel_unit_sptr &llu )
        {
            parent_->push_rpc_message( llu->id( ), llu );
        }

        void process_callback( lowlevel_unit_sptr &llu )
        {
            parent_->push_rpc_message( llu->target_id( ), llu );
        }

        void process_invalid( lowlevel_unit_sptr &llu )
        {
//            llu->Clear( );
//            parent_->send_message( *llu );
        }

        void on_ready( bool ready )
        {
            parent_->set_ready( ready );
            callbacks_->on_ready( );
        }

        void on_rpc_process( )
        {
            while( !parent_->message_queue_empty( ) ) {

                lowlevel_unit_sptr llu(vtrc::make_shared<lowlevel_unit_type>());

                bool check = parent_->parse_and_pop( *llu );

                if( !check ) {

                    llu->Clear( );
                    vtrc_errors::container *err = llu->mutable_error( );
                    err->set_code(vtrc_errors::ERR_PROTOCOL);
                    err->set_additional("Bad message was received");
                    err->set_fatal( true );

                    parent_->push_rpc_message_all( llu );
                    connection_->close( );

                    return;
                }

                switch( llu->info( ).message_type( ) ) {

                /// SERVER_CALL = request; do not use id
                case vtrc_rpc::message_info::MESSAGE_SERVER_CALL:
                    process_event( llu );
                    break;

                /// SERVER_CALLBACK = request; use target_id
                case vtrc_rpc::message_info::MESSAGE_SERVER_CALLBACK:
                    process_callback( llu );
                    break;

                /// internals; not implemented yet
                case vtrc_rpc::message_info::MESSAGE_SERVICE:
                    process_service( llu );
                    break;
                case vtrc_rpc::message_info::MESSAGE_INTERNAL:
                    process_internal( llu );
                    break;

                /// answers; use id
                case vtrc_rpc::message_info::MESSAGE_CLIENT_CALL:
                case vtrc_rpc::message_info::MESSAGE_CLIENT_CALLBACK:
                    process_call( llu );
                    break;
                default:
                    process_invalid( llu );
                }
            }
        }

        void on_server_ready( )
        {

            vtrc_auth::init_capsule capsule;
            bool check = parent_->parse_and_pop( capsule );

            if( !check ) {
                parent_->on_init_error(
                            create_error( vtrc_errors::ERR_INTERNAL, "" ),
                            "Server's 'Ready' has bad hash. Bad session key." );
                connection_->close( );
                return;
            }

            if( !capsule.ready( ) ) {
                parent_->on_init_error(capsule.error( ),
                                       "Server is not ready; stage: 'Ready'");
            }

            vtrc_auth::session_setup ss;
            ss.ParseFromString( capsule.body( ) );

            parent_->configure_session( ss.options( ) );

            change_stage( STAGE_RPC );

            on_ready( true );
        }

        void on_trans_setup( )
        {
            using namespace common::transformers;

            vtrc_auth::init_capsule capsule;
            bool check = parent_->parse_and_pop( capsule );

            if( !check ) {
                parent_->on_init_error(
                            create_error( vtrc_errors::ERR_INTERNAL, "" ),
                            "Server's 'Setup' has bad hash." );
                connection_->close( );
                return;
            }


            if( !capsule.ready( ) ) {
                parent_->on_init_error( capsule.error( ),
                                        "Server is not ready; stage: 'Setup'" );
                connection_->close( );
                return;
            }

            vtrc_auth::transformer_setup tsetup;

            tsetup.ParseFromString( capsule.body( ) );

            std::string key(client_->get_session_key( ));

            std::string s1(tsetup.salt1( ));
            std::string s2(tsetup.salt2( ));

            create_key( key, s1, s2, key );

            parent_->change_transformer( erseefor::create(
                                                 key.c_str( ), key.size( ) ) );

            key.assign( client_->get_session_key( ) );
            generate_key_infos( key, s1, s2, key );

            tsetup.set_salt1( s1 );
            tsetup.set_salt2( s2 );

            parent_->change_revertor( erseefor::create(
                                            key.c_str( ), key.size( ) ) );

            capsule.set_ready( true );
            capsule.set_body( tsetup.SerializeAsString( ) );

            change_stage( STAGE_READY );

            send_proto_message( capsule );

        }

        void set_options( const boost::system::error_code &err )
        {
            if( !err ) {
                parent_->change_hash_maker(
                   common::hash::create_by_index( vtrc_auth::HASH_CRC_32 ));
            }
        }

        static bool server_has_transformer( vtrc_auth::init_capsule const &cap,
                                            unsigned transformer_type )
        {
            vtrc_auth::init_protocol init;
            init.ParseFromString( cap.body( ) );
            return std::find( init.transform_supported( ).begin( ),
                      init.transform_supported( ).end( ),
                      transformer_type) != init.transform_supported( ).end( );
        }

        void on_hello_call( )
        {
            vtrc_auth::init_capsule capsule;
            bool check = parent_->parse_and_pop( capsule );

            if( !check ) {
                parent_->on_init_error(
                            create_error( vtrc_errors::ERR_INTERNAL, "" ),
                            "Server's 'Hello' has bad hash." );
                connection_->close( );
                return;
            }

            if( !capsule.ready( ) ) {
                parent_->on_init_error(capsule.error( ),
                                       "Server is not ready; stage: 'Hello'");
                connection_->close( );
                return;
            }

            vtrc_auth::client_selection init;

            capsule.set_ready( true );
            capsule.set_text( "Miten menee?" );

            bool key_set = client_->is_key_set( );

            const unsigned basic_transform = vtrc_auth::TRANSFORM_ERSEEFOR;

            if( key_set && server_has_transformer( capsule, basic_transform )) {
                init.set_transform( vtrc_auth::TRANSFORM_ERSEEFOR );
                init.set_id( client_->get_session_id( ) );
            } else {
                init.set_transform( vtrc_auth::TRANSFORM_NONE );
            }

            init.set_hash( vtrc_auth::HASH_CRC_32 );

            capsule.set_body( init.SerializeAsString( ) );

            parent_->change_hash_checker(
                    common::hash::create_by_index( vtrc_auth::HASH_CRC_32 ));

            change_stage( key_set ? STAGE_SETUP : STAGE_READY );

            send_proto_message( capsule,
                                vtrc::bind( &this_type::set_options, this, _1 ),
                                false );

        }

        void data_ready( )
        {
            stage_call_( );
        }

    };

    protocol_layer_c::protocol_layer_c(common::transport_iface   *connection,
                                        vtrc::client::vtrc_client *client ,
                                        protocol_signals *callbacks)
        :common::protocol_layer(connection, true)
        ,impl_(new impl(connection, client, callbacks))
    {
        impl_->parent_ = this;
    }

    protocol_layer_c::~protocol_layer_c( )
    {
        delete impl_;
    }

    void protocol_layer_c::init( )
    {
        impl_->init( );
    }

    void protocol_layer_c::close( )
    {
        impl_->check_disconnect_stage( );
        cancel_all_slots( );
        impl_->callbacks_->on_disconnect( );
    }

    const std::string &protocol_layer_c::client_id( ) const
    {
        return impl_->client_id( );
    }

    void protocol_layer_c::on_data_ready( )
    {
        impl_->data_ready( );
    }

    void protocol_layer_c::on_init_error(const vtrc_errors::container &error,
                                         const char *message)
    {
        impl_->callbacks_->on_init_error( error, message );
    }

    common::rpc_service_wrapper_sptr protocol_layer_c::get_service_by_name(
                                                        const std::string &name)
    {
        return impl_->get_service_by_name( name );
    }

}}
