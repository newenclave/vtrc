
#include "boost/asio.hpp"

#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-bind.h"
#include "vtrc-function.h"

#include "vtrc-protocol-layer-s.h"

#include "vtrc-common/vtrc-monotonic-timer.h"
#include "vtrc-common/vtrc-data-queue.h"
#include "vtrc-common/vtrc-hash-iface.h"
#include "vtrc-transformer-iface.h"

#include "vtrc-transport-iface.h"

#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-rpc-controller.h"
#include "vtrc-common/vtrc-call-context.h"

#include "vtrc-application.h"

#include "vtrc-errors.pb.h"
#include "vtrc-auth.pb.h"
#include "vtrc-rpc-lowlevel.pb.h"

#include "vtrc-chrono.h"
#include "vtrc-atomic.h"
#include "vtrc-common/vtrc-random-device.h"
#include "vtrc-common/vtrc-delayed-call.h"


namespace vtrc { namespace server {

    namespace gpb   = google::protobuf;
    namespace bsys  = boost::system;
    namespace basio = boost::asio;

#if 1
#define VPROTOCOL_S_LOCK_CONN( conn, ret )          \
    common::connection_iface_sptr lckd( conn );     \
    if( !lckd ) {                                   \
        return ret;                                 \
    }
#else
    #define VPROTOCOL_S_LOCK_CONN( conn, ret )
#endif

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

        typedef rpc::lowlevel_unit                   lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;
    }

    namespace data_queue = common::data_queue;

    struct protocol_layer_s::impl {

        typedef impl             this_type;
        typedef protocol_layer_s parent_type;

        application                     &app_;
        common::transport_iface         *connection_;
        common::connection_iface_wptr    keeper_;
        protocol_layer_s                *parent_;
        bool                             ready_;

        service_map                      services_;
        shared_mutex                     services_lock_;

        std::string                      client_id_;

        vtrc::atomic<unsigned>           current_calls_;

        common::delayed_call             keepalive_calls_;

        typedef vtrc::function<void (void)> stage_function_type;
        stage_function_type      stage_function_;

        impl( application &a, common::transport_iface *c )
            :app_(a)
            ,connection_(c)
            ,keeper_(c->weak_from_this( ))
            ,ready_(false)
            ,current_calls_(0)
            ,keepalive_calls_(a.get_io_service( ))
        {
            stage_function_ =
                    vtrc::bind( &this_type::on_client_selection, this );

            /// client has only 10 seconds for init connection
            /// todo: think about setting  for this timeout value
            keepalive_calls_.call_from_now(
                        vtrc::bind( &this_type::on_init_timeout, this,
                                     vtrc::placeholders::_1 ),
                        boost::posix_time::seconds( 10 ));
        }

        common::connection_iface_sptr lock_connection( )
        {
            common::connection_iface_sptr lckd(keeper_.lock( ));
            return lckd;
        }

        common::rpc_service_wrapper_sptr get_service( const std::string &name )
        {
            upgradable_lock lk( services_lock_ );
            common::rpc_service_wrapper_sptr result;
            service_map::iterator f( services_.find( name ) );

            //VPROTOCOL_S_LOCK_CONN( lock_connection( ), result );

            if( f != services_.end( ) ) {
                result = f->second;
            } else {
                result = app_.get_service_by_name( connection_, name );
                if( result ) {
                    upgrade_to_unique ulk( lk );
                    services_.insert( std::make_pair( name, result ) );
                }
            }

            return result;
        }

        const std::string &client_id( ) const
        {
            return client_id_;
        }

        void on_init_timeout( const boost::system::error_code &error )
        {
            if( !error ) {
                /// timeout for client init
                rpc::auth::init_capsule cap;
                cap.mutable_error( )->set_code( rpc::errors::ERR_TIMEOUT );
                cap.set_ready( false );
                send_and_close( cap );
            }
        }

        void send_proto_message( const gpb::MessageLite &mess )
        {
            //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );
            std::string s( mess.SerializeAsString( ) );
            connection_->write( s.c_str( ), s.size( ) );
        }

        void send_proto_message( const gpb::MessageLite &mess,
                                 common::system_closure_type closure,
                                 bool on_send)
        {
            //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );
            std::string s( mess.SerializeAsString( ) );
            connection_->write( s.c_str( ), s.size( ), closure, on_send );
        }

        void send_and_close( const gpb::MessageLite &mess )
        {
            VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );
            DEBUG_LINE(connection_);

            send_proto_message( mess,
                                vtrc::bind( &this_type::close_client, this,
                                             vtrc::placeholders::_1,
                                             connection_->weak_from_this( )),
                                true );
        }

        void set_client_ready( rpc::auth::init_capsule &capsule )
        {
            //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );

            keepalive_calls_.cancel( );
            stage_function_ =
                    vtrc::bind( &this_type::on_rcp_call_ready, this );

            capsule.set_ready( true );
            capsule.set_text( "Kiva nahda sinut!" );

            rpc::auth::session_setup session_setup;

            session_setup.mutable_options( )
                         ->CopyFrom( parent_->session_options( ) );

            app_.configure_session( connection_,
                                   *session_setup.mutable_options( ) );

            capsule.set_body( session_setup.SerializeAsString( ) );

            parent_->set_ready( true );

            send_proto_message( capsule );

        }

        void close_client( const bsys::error_code &      /*err */,
                           common::connection_iface_wptr &inst )
        {
            common::connection_iface_sptr lcked( inst.lock( ) );
            if( lcked ) {
                lcked->close( );
//                unique_shared_lock lk( services_lock_ );
//                services_.clear( );
            }
        }

        void on_client_transformer( )
        {
            //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );

            using namespace common::transformers;
            rpc::auth::init_capsule capsule;
            bool check = get_pop_message( capsule );

            if( !check ) {
                capsule.Clear( );
                capsule.mutable_error( )->set_code( rpc::errors::ERR_INTERNAL );
                send_and_close( capsule );
                return;
            }

            if( !capsule.ready( ) ) {
                connection_->close( );
                return;
            }

            rpc::auth::transformer_setup tsetup;

            tsetup.ParseFromString( capsule.body( ) );

            std::string key(app_.get_session_key( connection_, client_id_ ));

            create_key( key,             // input
                        tsetup.salt1( ), // input
                        tsetup.salt2( ), // input
                        key );           // output

            // client revertor is my transformer
            parent_->change_transformer( erseefor::create(
                                            key.c_str( ), key.size( ) ) );

            capsule.Clear( );

            set_client_ready( capsule );
        }

        void setup_transformer( unsigned id )
        {
            //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );

            using namespace common::transformers;

            rpc::auth::transformer_setup ts;
            rpc::auth::init_capsule capsule;

            if( id == rpc::auth::TRANSFORM_NONE ) {

                set_client_ready( capsule );

            } else if( id == rpc::auth::TRANSFORM_ERSEEFOR ) {

                std::string key(app_.get_session_key(connection_, client_id_));

                generate_key_infos( key,                 // input
                                   *ts.mutable_salt1( ), // output
                                   *ts.mutable_salt2( ), // output
                                    key );               // output

                // client transformer is my revertor
                parent_->change_revertor( erseefor::create(
                                              key.c_str( ), key.size( ) ) );

                capsule.set_ready( true );
                capsule.set_body( ts.SerializeAsString( ) );

                stage_function_ =
                        vtrc::bind( &this_type::on_client_transformer, this );

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

        void on_client_selection( )
        {
            //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );

            rpc::auth::init_capsule capsule;
            bool check = get_pop_message( capsule );

            if( !check ) {
                connection_->close( );
                return;
            }

            if( !capsule.ready( ) ) {
                connection_->close( );
                return;
            }

            rpc::auth::client_selection cs;
            cs.ParseFromString( capsule.body( ) );

            vtrc::ptr_keeper<common::hash_iface> new_checker (
                        common::hash::create_by_index( cs.hash( ) ) );

            vtrc::ptr_keeper<common::hash_iface> new_maker(
                        common::hash::create_by_index( cs.hash( ) ) );

            if( !new_maker.get( ) || !new_checker.get( ) ) {
                connection_->close( );
                return;
            }

            client_id_.assign( cs.id( ) );
            parent_->change_hash_checker( new_checker.release( ) );
            parent_->change_hash_maker( new_maker.release( ) );

            setup_transformer( cs.transform( ) );

        }

        bool get_pop_message( gpb::MessageLite &capsule )
        {
            //VPROTOCOL_S_LOCK_CONN( lock_connection( ), false );
            bool check = parent_->parse_and_pop( capsule );
            if( !check ) {
                connection_->close( );
            }
            return check;
        }

        void call_done( const rpc::errors::container & /*err*/ )
        {
            --current_calls_;
        }

        void push_call( lowlevel_unit_sptr llu,
                        common::connection_iface_sptr /*conn*/ )
        {
            parent_->make_local_call( llu,
                vtrc::bind(&this_type::call_done, this,
                            vtrc::placeholders::_1 ));
        }

        void send_busy( lowlevel_unit_type &llu )
        {
            if( llu.opt( ).wait( ) ) {
                llu.clear_call( );
                llu.clear_request( );
                llu.clear_response( );
                llu.mutable_error( )->set_code( rpc::errors::ERR_BUSY );
                parent_->call_rpc_method( llu );
            }
        }

        unsigned max_calls( ) const
        {
            return parent_->session_options( ).max_active_calls( );
        }

        void process_call( lowlevel_unit_sptr &llu )
        {
            //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );
            DEBUG_LINE(connection_);

            if( ++current_calls_ <= max_calls( ) ) {
                app_.get_rpc_service( ).post(
                        vtrc::bind( &this_type::push_call, this,
                                    llu, connection_->shared_from_this( )));
            } else {
                send_busy( *llu );
                --current_calls_;
            }
        }

        void push_event_answer( lowlevel_unit_sptr llu,
                                common::connection_iface_sptr /*conn*/ )
        {
            parent_->push_rpc_message( llu->id( ), llu );
        }

        void process_event_cb( lowlevel_unit_sptr &llu )
        {
            parent_->push_rpc_message( llu->id( ), llu );
        }

        void process_insertion( lowlevel_unit_sptr &llu )
        {
            parent_->push_rpc_message( llu->target_id( ), llu );
        }

        void on_rcp_call_ready( )
        {
            typedef rpc::message_info message_info;
            while( !parent_->message_queue_empty( ) ) {

                lowlevel_unit_sptr llu(vtrc::make_shared<lowlevel_unit_type>());

                if( !get_pop_message( *llu ) ) {
                    return;
                }

                if( llu->has_info( ) ) {

                    switch ( llu->info( ).message_type( ) ) {

                    /// CLIENT_CALL = request; do not change id
                    case message_info::MESSAGE_CLIENT_CALL:
                        process_call( llu );
                        break;

                    /// CLIENT_CALLBACK = request; must use target_id
                    case message_info::MESSAGE_CLIENT_CALLBACK:
                        process_insertion( llu );
                        break;

                    /// answers;
                    case message_info::MESSAGE_SERVER_CALL:
                    case message_info::MESSAGE_SERVER_CALLBACK:
                        process_event_cb( llu );
                        break;
                    default:
                        break;
                    }
                }
            }
        }

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

        void init( )
        {
            //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );
            static const std::string data(first_message( ));
            connection_->write(data.c_str( ), data.size( ));
        }

        void init_success( common::system_closure_type clos )
        {
            static const std::string data(first_message( ));
            connection_->write(data.c_str( ), data.size( ), clos, true );
        }

        void data_ready( )
        {
            common::connection_iface_sptr lckd( keeper_.lock( ) );
            if( !lckd ) {
                return;
                //throw std::runtime_error( "failed" );
            }
            //VPROTOCOL_S_LOCK_CONN( lock_connection( ),  );
            stage_function_( );
        }

    };

    protocol_layer_s::protocol_layer_s( application &a,
                                        common::transport_iface *connection,
                                        const rpc::session_options &opts )
        :common::protocol_layer(connection, false, opts )
        ,impl_(new impl(a, connection ))
    {
        impl_->parent_ = this;
    }

    protocol_layer_s::~protocol_layer_s( )
    {
        delete impl_;
    }

    void protocol_layer_s::init( )
    {
        impl_->init( );
    }

    void protocol_layer_s::init_success( common::system_closure_type clos )
    {
        return impl_->init_success( clos );
    }

    void protocol_layer_s::close( )
    {

    }

    void protocol_layer_s::send_and_close(const gpb::MessageLite &mess)
    {
        impl_->send_and_close( mess );
    }

    const std::string &protocol_layer_s::client_id( ) const
    {
        return impl_->client_id( );
    }

    void protocol_layer_s::on_data_ready( )
    {
        impl_->data_ready( );
    }

    common::rpc_service_wrapper_sptr protocol_layer_s::get_service_by_name(
                                                        const std::string &name)
    {
        return impl_->get_service(name);
    }


}}

