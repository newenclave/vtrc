
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

#include "vtrc-application.h"

#include "vtrc-memory.h"
#include "vtrc-bind.h"

#include "boost/asio.hpp"

#include <stdlib.h>

namespace vtrc { namespace server {

    namespace bs = boost::system;
    namespace ba = boost::asio;
    namespace gpb = google::protobuf;

    namespace {

        void default_cb( bs::error_code const & )
        {

        }

        typedef vtrc::function<void (const std::string &)> stage_function_type;

        std::string first_message( )
        {
            rpc::auth::init_capsule cap;
            rpc::auth::init_protocol hello_mess;
            cap.set_text( "Tervetuloa!" );
            cap.set_ready( true );

            hello_mess.add_hash_supported( rpc::auth::HASH_NONE     );
            hello_mess.add_hash_supported( rpc::auth::HASH_CRC_16   );
            hello_mess.add_hash_supported( rpc::auth::HASH_CRC_32   );
            hello_mess.add_hash_supported( rpc::auth::HASH_CRC_64   );
            hello_mess.add_hash_supported( rpc::auth::HASH_SHA2_256 );

            hello_mess.add_transform_supported( rpc::auth::TRANSFORM_NONE );
            hello_mess.add_transform_supported( rpc::auth::TRANSFORM_ERSEEFOR );

            cap.set_body( hello_mess.SerializeAsString( ) );

            return cap.SerializeAsString( );
        }

        struct iface: public common::connection_setup_iface {

            common::protocol_accessor *pa_;
            application               &app_;
            rpc::errors::container     err_;
            bool                       failed_;
            bool                       ready_;
            common::delayed_call       keepalive_calls_;
            stage_function_type        stage_function_;
            std::string                client_id_;

            iface( application &a )
                :pa_(NULL)
                ,app_(a)
                ,failed_(false)
                ,ready_(false)
                ,keepalive_calls_(a.get_io_service( ))
            {
                stage_function_ =
                        vtrc::bind( &iface::on_client_selection, this,
                                    vtrc::placeholders::_1 );
            }

            common::connection_iface *conn( )
            {
                return pa_->connection( );
            }

            void on_init_timeout( const bs::error_code &error )
            {
                if( !error ) {
                    failed_ = true;
                    err_.set_code( rpc::errors::ERR_TIMEOUT );
                }
            }

            void send_proto_message( const gpb::MessageLite &mess )
            {
                std::string s( mess.SerializeAsString( ) );
                write( s );
            }

            void send_proto_message( const gpb::MessageLite &mess,
                                     common::system_closure_type closure,
                                     bool on_send)
            {
                std::string s( mess.SerializeAsString( ) );
                pa_->write( s, closure, on_send );
            }

            void close_client( const bs::error_code &      /*err */,
                               common::connection_iface_wptr &inst )
            {
                common::connection_iface_sptr lcked( inst.lock( ) );
                ready_ = true;
                if( lcked ) {
                    lcked->close( );
                }
            }

            void send_and_close( const gpb::MessageLite &mess )
            {
                send_proto_message( mess,
                                    vtrc::bind( &iface::close_client, this,
                                                 vtrc::placeholders::_1,
                                                 conn( )->weak_from_this( ) ),
                                    true );
            }


            void set_client_ready( rpc::auth::init_capsule &capsule )
            {
                //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );

                keepalive_calls_.cancel( );

                capsule.set_ready( true );
                capsule.set_text( "Kiva nahda sinut!" );

                rpc::auth::session_setup session_setup;

//                session_setup.mutable_options( )
//                             ->CopyFrom( parent_->session_options( ) );

                app_.configure_session( conn( ),
                                       *session_setup.mutable_options( ) );

                capsule.set_body( session_setup.SerializeAsString( ) );

                ready_ = true;
                //parent_->set_ready( true );

                send_proto_message( capsule );

            }

            void on_client_transformer( const std::string &data )
            {
                //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );

                using namespace common::transformers;
                rpc::auth::init_capsule capsule;

                if(!capsule.ParseFromString( data )) {
                    pa_->close( );
                    return;
                }

                if( !capsule.ready( ) ) {
                    conn( )->close( );
                    return;
                }

                rpc::auth::transformer_setup tsetup;

                tsetup.ParseFromString( capsule.body( ) );

                std::string key(app_.get_session_key( conn( ), client_id_ ));

                create_key( key,             // input
                            tsetup.salt1( ), // input
                            tsetup.salt2( ), // input
                            key );           // output

                // client revertor is my transformer
                pa_->set_transformer( erseefor::create(
                                            key.c_str( ), key.size( ) ) );

                capsule.Clear( );

                set_client_ready( capsule );

            }

            void setup_transformer( unsigned id )
            {
                //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );

                using namespace common::transformers;

                rpc::auth::init_capsule capsule;

                if( id == rpc::auth::TRANSFORM_NONE ) {

                    if( !app_.session_key_required( conn( ), client_id_ ) ) {
                        set_client_ready( capsule );
                    } else {
                        capsule.set_ready( false );
                        rpc::errors::container *er(capsule.mutable_error( ));
                        er->set_code( rpc::errors::ERR_ACCESS );
                        er->set_category( rpc::errors::CATEGORY_INTERNAL );
                        er->set_additional( "Session key required" );
                        send_and_close( capsule );
                        return;
                    }

                } else if( id == rpc::auth::TRANSFORM_ERSEEFOR ) {

                    rpc::auth::transformer_setup ts;
                    std::string key(app_.get_session_key( conn( ), client_id_));

                    generate_key_infos( key,                 // input
                                       *ts.mutable_salt1( ), // output
                                       *ts.mutable_salt2( ), // output
                                        key );               // output

                    // client transformer is my revertor
                    pa_->set_revertor( erseefor::create(
                                                key.c_str( ), key.size( ) ) );
                    //std::cout << "Set revertor " << key.c_str( ) << "\n";

                    capsule.set_ready( true );
                    capsule.set_body( ts.SerializeAsString( ) );

                    stage_function_ =
                            vtrc::bind( &iface::on_client_transformer, this,
                                        vtrc::placeholders::_1 );

                    send_proto_message( capsule );

                } else {
                    capsule.set_ready( false );
                    rpc::errors::container *er(capsule.mutable_error( ));
                    er->set_code( rpc::errors::ERR_INVALID_VALUE );
                    er->set_category( rpc::errors::CATEGORY_INTERNAL );
                    er->set_additional( "Invalid transformer" );
                    send_and_close( capsule );
                    return;
                }
            }

            void on_client_selection( const std::string &data )
            {
                //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );
                rpc::auth::init_capsule capsule;

                if(!capsule.ParseFromString( data )) {
                    pa_->close( );
                    return;
                }

                if( !capsule.ready( ) ) {
                    failed_ = true;
                    return;
                }

                rpc::auth::client_selection cs;
                cs.ParseFromString( capsule.body( ) );

                vtrc::ptr_keeper<common::hash_iface> new_checker (
                            common::hash::create_by_index( cs.hash( ) ) );

                vtrc::ptr_keeper<common::hash_iface> new_maker(
                            common::hash::create_by_index( cs.hash( ) ) );

                if( !new_maker.get( ) || !new_checker.get( ) ) {
                    failed_ = true;
                    return;
                }

                client_id_.assign( cs.id( ) );
                pa_->set_client_id( cs.id( ) );
                pa_->set_hash_checker( new_checker.release( ) );
                pa_->set_hash_maker( new_maker.release( ) );

                setup_transformer( cs.transform( ) );

            }

            void write( const std::string &data )
            {
                pa_->write( data, default_cb, true );
            }

            void init( common::protocol_accessor *pa,
                       common::system_closure_type cb )
            {
                static const std::string data(first_message( ));
                pa_ = pa;
                pa_->write( data, cb, true );
                keepalive_calls_.call_from_now(
                            vtrc::bind( &iface::on_init_timeout, this,
                                         vtrc::placeholders::_1 ),
                            boost::posix_time::seconds( 10 ));

            }

            bool next( const std::string &data )
            {
                stage_function_( data );
                return !ready_;
            }

            bool close_connection( )
            {
                return failed_;
            }
        };
    }

    common::connection_setup_iface *create_default_setup( application &a )
    {
        return new iface( a );
    }

}}

