
#include "boost/asio.hpp"

#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-bind.h"
#include "vtrc-function.h"

#include "vtrc-protocol-layer-s.h"

#include "vtrc-monotonic-timer.h"
#include "vtrc-data-queue.h"
#include "vtrc-hash-iface.h"
#include "vtrc-transformer-iface.h"

#include "vtrc-transport-iface.h"

#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-rpc-controller.h"
#include "vtrc-common/vtrc-call-context.h"

#include "vtrc-application.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-auth.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-chrono.h"
#include "vtrc-atomic.h"
#include "vtrc-common/vtrc-random-device.h"
#include "vtrc-common/vtrc-delayed-call.h"


namespace vtrc { namespace server {

    namespace gpb   = google::protobuf;
    namespace bsys  = boost::system;
    namespace basio = boost::asio;

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

        typedef vtrc_rpc::lowlevel_unit     lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;
    }

    namespace data_queue = common::data_queue;

    struct protocol_layer_s::impl {

        typedef impl             this_type;
        typedef protocol_layer_s parent_type;

        application             &app_;
        common::transport_iface *connection_;
        protocol_layer_s        *parent_;
        bool                     ready_;

        service_map              services_;
        shared_mutex             services_lock_;

        std::string              client_id_;

        common::delayed_call     keepalive_calls_;

        typedef vtrc::function<void (void)> stage_function_type;
        stage_function_type      stage_function_;

        vtrc::atomic<unsigned>   current_calls_;
        const unsigned           maximum_calls_;

        impl( application &a, common::transport_iface *c,
              unsigned maximum_calls)
            :app_(a)
            ,connection_(c)
            ,ready_(false)
            ,current_calls_(0)
            ,maximum_calls_(maximum_calls)
            ,keepalive_calls_(a.get_io_service( ))
        {
            stage_function_ =
                    vtrc::bind( &this_type::on_client_selection, this );

            /// client has only 10 seconds for init connection
            /// todo: think about setting  for this timeout value
            keepalive_calls_.call_from_now(
                        vtrc::bind( &this_type::on_init_timeout, this, _1 ),
                        boost::posix_time::seconds( 10 ));
        }

        common::rpc_service_wrapper_sptr get_service( const std::string &name )
        {
            upgradable_lock lk( services_lock_ );
            common::rpc_service_wrapper_sptr result;
            service_map::iterator f( services_.find( name ) );

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
                vtrc_auth::init_capsule cap;
                cap.mutable_error( )->set_code( vtrc_errors::ERR_TIMEOUT );
                cap.set_ready( false );
                send_and_close( cap );
            }
        }

        void send_proto_message( const gpb::MessageLite &mess )
        {
            std::string s(mess.SerializeAsString( ));
            connection_->write( s.c_str( ), s.size( ) );
        }

        void send_proto_message( const gpb::MessageLite &mess,
                             common::system_closure_type closure, bool on_send)
        {
            std::string s(mess.SerializeAsString( ));
            connection_->write( s.c_str( ), s.size( ), closure, on_send );
        }

        void send_and_close( const gpb::MessageLite &mess )
        {
            send_proto_message( mess,
                                vtrc::bind( &this_type::close_client, this, _1,
                                            connection_->shared_from_this( )),
                                true );
        }

        void set_client_ready(  )
        {
            keepalive_calls_.cancel( );
            stage_function_ =
                    vtrc::bind( &this_type::on_rcp_call_ready, this );
            parent_->set_ready( true );
        }

        void close_client( const bsys::error_code & /*err*/,
                           common::connection_iface_sptr /*inst*/)
        {
            connection_->close( );
        }

        void on_client_transformer( )
        {
            using namespace common::transformers;
            vtrc_auth::init_capsule capsule;
            bool check = get_pop_message( capsule );

            if( !check ) {
                capsule.Clear( );
                capsule.mutable_error( )->set_code( vtrc_errors::ERR_INTERNAL );
                send_and_close( capsule );
                return;
            }

            if( !capsule.ready( ) ) {
                connection_->close( );
                return;
            }

            vtrc_auth::transformer_setup tsetup;

            tsetup.ParseFromString( capsule.body( ) );

            std::string key(app_.get_session_key( connection_, client_id_ ));

            create_key( key,                // input
                        tsetup.salt1( ),    // input
                        tsetup.salt2( ),    // input
                        key );              // output

            // client revertor is my transformer
            parent_->change_transformer( erseefor::create(
                                                key.c_str( ), key.size( ) ) );

            capsule.Clear( );
            capsule.set_ready( true );
            capsule.set_text( "Kiva nahda sinut!" );

            set_client_ready(  );

            send_proto_message( capsule );

        }

        void setup_transformer( unsigned id )
        {
            using namespace common::transformers;

            vtrc_auth::transformer_setup ts;
            vtrc_auth::init_capsule capsule;

            if( id == vtrc_auth::TRANSFORM_NONE ) {

                capsule.set_ready( true );
                capsule.set_text( "Kiva nahda sinut!" );

                set_client_ready(  );

                send_proto_message( capsule );

            } else if( id == vtrc_auth::TRANSFORM_ERSEEFOR ) {

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
                vtrc_errors::container *er(capsule.mutable_error( ));
                er->set_code( vtrc_errors::ERR_INVALID_VALUE );
                er->set_category(vtrc_errors::CATEGORY_INTERNAL);
                er->set_additional( "Invalid transformer" );
                send_and_close( capsule );
                return;
            }
        }

        void on_client_selection( )
        {
            vtrc_auth::init_capsule capsule;
            bool check = get_pop_message( capsule );

            if( !check ) {
                connection_->close( );
                return;
            }

            if( !capsule.ready( ) ) {
                connection_->close( );
                return;
            }

            vtrc_auth::client_selection cs;
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
            bool check = parent_->parse_and_pop( capsule );
            if( !check ) {
                connection_->close( );
            }
            return check;
        }

        void call_done( const vtrc_errors::container & /*err*/ )
        {
            --current_calls_;
        }

        void push_call( lowlevel_unit_sptr llu,
                        common::connection_iface_sptr /*conn*/ )
        {
            parent_->make_local_call( llu,
                vtrc::bind(&this_type::call_done, this, _1 ));
        }

        void send_busy( lowlevel_unit_type &llu )
        {
            if( llu.opt( ).wait( ) ) {
                llu.clear_call( );
                llu.clear_request( );
                llu.clear_response( );
                llu.mutable_error( )->set_code( vtrc_errors::ERR_BUSY );
                parent_->call_rpc_method( llu );
            }
        }

        void process_call( lowlevel_unit_sptr &llu )
        {
            if( ++current_calls_ <= maximum_calls_ ) {
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
            typedef vtrc_rpc::message_info message_info;
            while( !parent_->message_queue_empty( ) ) {

                lowlevel_unit_sptr llu(vtrc::make_shared<lowlevel_unit_type>());

                if(!get_pop_message( *llu )) {
                    return;
                }

                if( llu->has_info( ) ) {

                    switch ( llu->info( ).message_type( ) ) {

                    /// CALL = request; do not change id
                    case message_info::MESSAGE_CALL:
                        process_call( llu );
                        break;

                    /// INSERTION_CALL = request; must use target_id
                    case message_info::MESSAGE_INSERTION_CALL:
                        process_insertion( llu );
                        break;

                    /// answers;
                    case message_info::MESSAGE_EVENT:
                    case message_info::MESSAGE_CALLBACK:
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
            vtrc_auth::init_capsule cap;
            vtrc_auth::init_protocol hello_mess;
            cap.set_text( "Tervetuloa!" );
            cap.set_ready( true );

            hello_mess.add_hash_supported( vtrc_auth::HASH_NONE     );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_16   );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_32   );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_64   );
            hello_mess.add_hash_supported( vtrc_auth::HASH_SHA2_256 );

            hello_mess.add_transform_supported( vtrc_auth::TRANSFORM_NONE );
            hello_mess.add_transform_supported( vtrc_auth::TRANSFORM_ERSEEFOR );

            cap.set_body( hello_mess.SerializeAsString( ) );

            return cap.SerializeAsString( );
        }

        void init( )
        {
            static const std::string data(first_message( ));
            connection_->write(data.c_str( ), data.size( ));
        }

        void data_ready( )
        {
            stage_function_( );
        }

    };

    protocol_layer_s::protocol_layer_s( application &a,
                                        common::transport_iface *connection,
                                        unsigned maximym_calls,
                                        size_t mess_len)
        :common::protocol_layer(connection, false, mess_len)
        ,impl_(new impl(a, connection, maximym_calls))
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

    void protocol_layer_s::close( )
    {

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

