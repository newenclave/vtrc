
#include "vtrc-common/vtrc-protocol-accessor-iface.h"
#include "vtrc-common/vtrc-delayed-call.h"
#include "vtrc-common/vtrc-hash-iface.h"
#include "vtrc-common/vtrc-transformer-iface.h"
#include "vtrc-common/vtrc-connection-iface.h"

#include "vtrc-default-lowlevel-protocol.h"

#include "vtrc-errors.pb.h"
#include "vtrc-auth.pb.h"
#include "vtrc-rpc-lowlevel.pb.h"
#include "vtrc-function.h"

#include "vtrc-application.h"

#include "vtrc-memory.h"
#include "vtrc-bind.h"

#include "vtrc-asio.h"

#include <stdlib.h>

namespace vtrc { namespace server {

    namespace bs = VTRC_SYSTEM;
    namespace ba = VTRC_ASIO;
    namespace gpb = google::protobuf;

    namespace {

        void default_cb( bs::error_code const & )
        { }

        namespace default_cypher = common::transformers::chacha;

        typedef vtrc::function<void (const std::string &)> stage_function_type;

        typedef vtrc::unique_ptr<common::delayed_call> delayed_call_ptr;

        std::string first_message( )
        {
            rpc::auth::init_capsule cap;
            rpc::auth::init_protocol hello_mess;
            cap.set_text( "Tervetuloa!" );
            cap.set_ready( true );

            hello_mess.add_hash_supported( rpc::auth::HASH_NONE     );
            hello_mess.add_hash_supported( rpc::auth::HASH_CRC_16   );
            hello_mess.add_hash_supported( rpc::auth::HASH_CRC_32   );
            hello_mess.add_hash_supported( rpc::auth::HASH_SHA2_256 );

            hello_mess.add_transform_supported( rpc::auth::TRANSFORM_NONE );
            hello_mess.add_transform_supported( rpc::auth::TRANSFORM_ERSEEFOR );

            cap.set_body( hello_mess.SerializeAsString( ) );

            return cap.SerializeAsString( );
        }

        struct iface: common::lowlevel::default_protocol {

            typedef common::lowlevel::default_protocol parent_type;

            application               &app_;
            bool                       ready_;
            delayed_call_ptr           keepalive_calls_;
            stage_function_type        stage_function_;
            std::string                client_id_;
            rpc::session_options       session_opts_;

            iface( application &a, const rpc::session_options &opts )
                :app_(a)
                ,ready_(false)
                ,keepalive_calls_(new common::delayed_call(a.get_io_service( )))
                ,session_opts_(opts)
            {
                stage_function_ =
                        vtrc::bind( &iface::on_client_selection, this,
                                    vtrc::placeholders::_1 );
            }

            common::connection_iface *conn( )
            {
                return accessor( )->connection( );
            }

            void close( )
            {

            }

            void on_init_timeout( const bs::error_code &error )
            {
                if( !error && !ready_ ) {
                    /// timeout for client init
                    rpc::auth::init_capsule cap;
                    cap.mutable_error( )->set_code( rpc::errors::ERR_TIMEOUT );
                    cap.set_ready( false );
                    send_and_close( cap );
                }
            }

            void send_proto_message( const gpb::MessageLite &mess )
            {
                std::string s( mess.SerializeAsString( ) );
                //s = pack_message( s.c_str( ), s.size( ) );
                write( s );
            }

            void send_proto_message( const gpb::MessageLite &mess,
                                     common::system_closure_type closure,
                                     bool on_send)
            {
                std::string s( mess.SerializeAsString( ) );
                //s = pack_message( s.c_str( ), s.size( ) );
                accessor( )->write( s, closure, on_send );
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
                keepalive_calls_.reset( );

                capsule.set_ready( true );
                capsule.set_text( "Kiva nahda sinut!" );


                rpc::auth::session_setup session_setup;

                session_setup.mutable_options( )
                             ->CopyFrom( session_opts_ );

                app_.configure_session( conn( ),
                                       *session_setup.mutable_options( ) );

                configure( session_setup.options( ) );
                capsule.set_body( session_setup.SerializeAsString( ) );

                ready_ = true;

                send_proto_message( capsule );

            }

            void on_client_transformer( const std::string &data )
            {
                using namespace common::transformers;
                rpc::auth::init_capsule capsule;

                if(!capsule.ParseFromString( data )) {
                    accessor( )->close( );
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
                set_transformer( default_cypher::create(
                                 key.c_str( ), key.size( ) ) );

                capsule.Clear( );

                set_client_ready( capsule );

            }

            void setup_transformer( unsigned id )
            {
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
                    set_revertor( default_cypher::create( key.c_str( ),
                                                          key.size( ) ) );
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

                rpc::auth::init_capsule capsule;

                if(!capsule.ParseFromString( data )) {
                    accessor( )->close( );
                    return;
                }

                if( !capsule.ready( ) ) {
                    return;
                }

                rpc::auth::client_selection cs;
                cs.ParseFromString( capsule.body( ) );

                vtrc::ptr_keeper<common::hash_iface> new_checker (
                            common::hash::create_by_index( cs.hash( ) ) );

                vtrc::ptr_keeper<common::hash_iface> new_maker(
                            common::hash::create_by_index( cs.hash( ) ) );

                if( !new_maker.get( ) || !new_checker.get( ) ) {
                    accessor( )->close( );
                    return;
                }

                client_id_.assign( cs.id( ) );
                accessor( )->set_client_id( cs.id( ) );

                set_hash_checker( new_checker.release( ) );
                set_hash_maker( new_maker.release( ) );

                setup_transformer( cs.transform( ) );

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

            void write( const std::string &data )
            {
                accessor( )->write( data, default_cb, true );
            }

            void init( common::protocol_accessor *pa,
                       common::system_closure_type cb )
            {
                set_accessor( pa );
                switch_to_handshake( );
                static const std::string data(first_message( ));
//                ready_ = true;
//                pa_->ready( true );
//                cb( VTRC_SYSTEM::error_code( ) );
//                return;

                const unsigned to = session_opts_.init_timeout( );

                accessor( )->write( data, cb, true );
                keepalive_calls_->call_from_now(
                            vtrc::bind( &iface::on_init_timeout, this,
                                         vtrc::placeholders::_1 ),
                            boost::posix_time::milliseconds( to ));

            }

            void do_handshake(  )
            {
                std::string data;
                if( !pop_raw_message( data ) ) {
                    accessor( )->error( create_error(
                                        rpc::errors::ERR_INTERNAL, "" ),
                                        "Bad hash." );
                    accessor( )->close( );
                } else {
                    stage_function_( data );
                    if( ready_ ) {
                        switch_to_ready( );
                        accessor( )->ready( ready_ );
                    }
                }
            }

            bool ready( ) const
            {
                return ready_;
            }

        };
    }

    common::lowlevel::protocol_layer_iface *create_default_setup(
                      application &a, const rpc::session_options &opts )
    {
        return new iface( a, opts );
    }

}}

