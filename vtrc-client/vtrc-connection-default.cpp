
#include "vtrc-common/vtrc-connection-setup-iface.h"
#include "vtrc-common/vtrc-protocol-accessor-iface.h"
#include "vtrc-common/vtrc-delayed-call.h"
#include "vtrc-common/vtrc-hash-iface.h"
#include "vtrc-common/vtrc-transformer-iface.h"
#include "vtrc-common/vtrc-connection-iface.h"

#include "vtrc-errors.pb.h"
#include "vtrc-auth.pb.h"
#include "vtrc-rpc-lowlevel.pb.h"
#include "vtrc-function.h"

#include "vtrc-memory.h"
#include "vtrc-bind.h"

#include "vtrc-client.h"

#include "boost/asio.hpp"

#include <stdlib.h>

namespace vtrc { namespace client {

    namespace bs = boost::system;
    namespace ba = boost::asio;
    namespace gpb = google::protobuf;

    namespace {

        enum protocol_stage {
             STAGE_HELLO = 1
            ,STAGE_SETUP = 2
            ,STAGE_READY = 3
            ,STAGE_RPC   = 4
        };

        void default_cb( bs::error_code const & )
        { }

        typedef vtrc::function<void (const rpc::errors::container &,
                                     const char *)> init_error_cb;
        typedef vtrc::function<void (const std::string &)> stage_function_type;

        struct iface: common::connection_setup_iface {

            common::protocol_accessor  *pa_;
            bool                        ready_;
            stage_function_type         stage_call_;
            vtrc_client                *client_;
            protocol_stage              stage_;
            init_error_cb               init_error_;

            iface( vtrc_client *client, init_error_cb init_error )
                :pa_(NULL)
                ,ready_(false)
                ,client_(client)
                ,stage_(STAGE_HELLO)
            { }

            void send_proto_message( const gpb::MessageLite &mess,
                                     common::system_closure_type closure,
                                     bool on_send )
            {
                std::string s(mess.SerializeAsString( ));
                pa_->write( s, closure, on_send );
            }

            rpc::errors::container create_error( unsigned code,
                                                 const std::string &add )
            {
                rpc::errors::container cont;
                cont.set_code( code );
                cont.set_category( rpc::errors::CATEGORY_INTERNAL );
                cont.set_additional( add );
                return cont;
            }

            static
            bool server_has_transformer( rpc::auth::init_capsule const &cap,
                                         unsigned transformer_type )
            {
                rpc::auth::init_protocol init;
                init.ParseFromString( cap.body( ) );
                return std::find( init.transform_supported( ).begin( ),
                       init.transform_supported( ).end( ),
                       transformer_type ) != init.transform_supported( ).end( );
            }

            void change_stage( protocol_stage stage )
            {
                stage_ = stage;
                switch ( stage_ ) {
                case STAGE_HELLO:
                    stage_call_ = vtrc::bind( &iface::on_hello_call, this,
                                              vtrc::placeholders::_1 );
                    break;
//                case STAGE_SETUP:
//                    stage_call_ = vtrc::bind( &iface::on_trans_setup, this );
//                    break;
                case STAGE_READY:
                    stage_call_ = vtrc::bind( &iface::on_server_ready, this,
                                              vtrc::placeholders::_1 );
                    break;
//                case STAGE_RPC:
//                    stage_call_ = vtrc::bind( &iface::on_rpc_process, this );
//                    break;
                }
            }

            void set_options( const boost::system::error_code &err )
            {
                if( !err ) {
                    pa_->set_hash_maker(
                       common::hash::create_by_index( rpc::auth::HASH_CRC_32 ));
                }
            }

            void on_server_ready( const std::string &data )
            {

                rpc::auth::init_capsule capsule;
                bool check = capsule.ParseFromString( data );

                if( !check ) {
                    init_error_( create_error( rpc::errors::ERR_INTERNAL, "" ),
                           "Server's 'Ready' has bad hash. Bad session key." );
                    pa_->close( );
                    return;
                }

                if( !capsule.ready( ) ) {
                    if( capsule.error( ).has_additional( ) ) {
                        init_error_( capsule.error( ),
                                     capsule.error( ).additional( ).c_str( ) );

                    } else {
                        init_error_( capsule.error( ),
                                     "Server is not ready; stage: 'Ready'" );
                    }
                }

                rpc::auth::session_setup ss;
                ss.ParseFromString( capsule.body( ) );

                //parent_->configure_session( ss.options( ) );

                change_stage( STAGE_RPC );

                //on_ready( true );

            }

            void on_hello_call( const std::string &data )
            {

                rpc::auth::init_capsule capsule;

                bool check = capsule.ParseFromString( data );

                if( !check ) {
                    init_error_( create_error( rpc::errors::ERR_INTERNAL, "" ),
                                 "Server's 'Hello' has bad hash." );
                    pa_->close( );
                    return;
                }

                if( !capsule.ready( ) ) {
                    init_error_( capsule.error( ),
                                 "Server is not ready; stage: 'Hello'");
                    pa_->close( );
                    return;
                }

                rpc::auth::client_selection init;

                capsule.set_ready( true );
                capsule.set_text( "Miten menee?" );

                bool key_set = client_->is_key_set( );

                const unsigned basic_transform = rpc::auth::TRANSFORM_ERSEEFOR;

                if( key_set && server_has_transformer( capsule,
                                                       basic_transform ) ) {
                    init.set_transform( rpc::auth::TRANSFORM_ERSEEFOR );
                    init.set_id( client_->get_session_id( ) );
                } else {
                    init.set_transform( rpc::auth::TRANSFORM_NONE );
                }

                init.set_hash( rpc::auth::HASH_CRC_32 );

                capsule.set_body( init.SerializeAsString( ) );

                pa_->set_hash_checker(
                     common::hash::create_by_index( rpc::auth::HASH_CRC_32 ) );

                change_stage( key_set ? STAGE_SETUP : STAGE_READY );

                send_proto_message( capsule,
                                    vtrc::bind( &iface::set_options, this,
                                                 vtrc::placeholders::_1 ),
                                    false );

            }

            void init( common::protocol_accessor *pa,
                       common::system_closure_type cb )
            {
                pa_ = pa;
            }

            bool next( const std::string &data )
            {
                return !ready_;
            }
        };

    }

    common::connection_setup_iface *create_default_setup( vtrc_client *client,
                                    init_error_cb init_error )
    {
        return new iface( client, init_error );
    }

}}
