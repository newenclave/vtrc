#include <algorithm>
#include <iostream>

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

#include "vtrc-common/vtrc-lowlevel-protocol-iface.h"
#include "vtrc-common/vtrc-protocol-accessor-iface.h"

#include "vtrc-protocol-layer-c.h"

#include "vtrc-client.h"

#include "vtrc-auth.pb.h"
#include "vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace client {

    namespace gpb = google::protobuf;

    typedef vtrc::function<void (const rpc::errors::container &,
                                 const char *)> init_error_cb;

    typedef common::lowlevel_protocol_layer_iface lowlevel_protocol_layer_iface;
    lowlevel_protocol_layer_iface *create_default_setup( vtrc_client *c );

    namespace {


        typedef common::lowlevel_protocol_layer_iface connection_setup_iface;

        typedef connection_setup_iface *connection_setup_ptr;

        typedef vtrc::shared_ptr<rpc::lowlevel_unit> lowlevel_unit_sptr;

        typedef vtrc::shared_ptr<gpb::Service> service_str;

        enum protocol_stage {
             STAGE_HELLO = 1
            ,STAGE_SETUP = 2
            ,STAGE_READY = 3
            ,STAGE_RPC   = 4
        };

        //namespace default_cypher = common::transformers::chacha;

    }

    struct protocol_layer_c::impl: common::protocol_accessor {

        typedef impl this_type;
        typedef protocol_layer_c parent_type;

        typedef vtrc::function<void (void)> stage_funcion_type;

        common::transport_iface     *connection_;
        protocol_layer_c            *parent_;
        stage_funcion_type           stage_call_;
        vtrc_client                 *client_;
        protocol_signals            *callbacks_;
        protocol_stage               stage_;

        bool                         closed_;
        connection_setup_ptr         conn_setup_;
        lowlevel_factory_type        ll_factory_;

        impl( common::transport_iface *c, vtrc_client *client,
              protocol_signals *cb )
            :connection_(c)
            ,client_(client)
            ,callbacks_(cb)
            ,stage_(STAGE_HELLO)
            ,closed_(false)
        { }

        protocol_layer_c::lowlevel_factory_type create_default_factory( )
        {
            return vtrc::bind( create_default_setup, client_ );
        }

        //// ================ accessor =================

        void call_setup_function( )
        {
            conn_setup_->do_handshake( );
        }

        void set_client_id( const std::string &id )
        {
            ;;;
        }

        void error( const rpc::errors::container &err, const char *mes )
        {
            callbacks_->on_init_error( err, mes );
        }

        void write( const std::string &data,
                    common::system_closure_type cb,
                    bool on_send_success )
        {
            connection_->write( data.c_str( ), data.size( ),
                                cb, on_send_success );
        }

        common::connection_iface *connection( )
        {
            return connection_;
        }

        void ready( bool value )
        {
            stage_call_ = value
                        ? vtrc::bind( &this_type::on_rpc_process, this )
                        : vtrc::bind( &this_type::call_setup_function, this );
            on_ready( value );
        }

        void close( )
        {
            closed_ = true;
            connection_->close( );
        }

        void configure_session( const vtrc::rpc::session_options &opts )
        {
            parent_->configure_session( opts );
        }

        //// ================ accessor =================

        static void closure_none( const boost::system::error_code & )
        { }

        void init( )
        {
            conn_setup_ = ll_factory_( ); // create_default_setup( client_ );
            parent_->set_lowlevel( conn_setup_ );
            stage_call_ = vtrc::bind( &this_type::call_setup_function, this );
            conn_setup_->init( this, &this_type::closure_none );
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

        const std::string &client_id( ) const
        {
            return client_->get_session_id( );
        }

        typedef common::rpc_service_wrapper_sptr wrapper_sptr;

        wrapper_sptr get_service_by_name( const std::string &name )
        {
            vtrc::shared_ptr<gpb::Service> res(client_->get_rpc_handler(name));

            if( res ) {
                return wrapper_sptr (new common::rpc_service_wrapper( res ));
            } else {
                return wrapper_sptr( );
            }
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
            if( !c ) {
                return;
            }
            parent_->make_local_call( llu );
        }

        void process_event( lowlevel_unit_sptr &llu )
        {
            if ( llu->id( ) != 0 ) {
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
                    rpc::errors::container *err = llu->mutable_error( );
                    err->set_code( rpc::errors::ERR_PROTOCOL);
                    err->set_additional("Bad message was received");
                    err->set_fatal( true );

                    parent_->push_rpc_message_all( llu );
                    connection_->close( );

                    return;
                }

                switch( llu->info( ).message_type( ) ) {

                /// SERVER_CALL = request; do not use id
                case rpc::message_info::MESSAGE_SERVER_CALL:
                    process_event( llu );
                    break;

                /// SERVER_CALLBACK = request; use target_id
                case rpc::message_info::MESSAGE_SERVER_CALLBACK:
                    process_callback( llu );
                    break;

                /// internals; not implemented yet
                case rpc::message_info::MESSAGE_SERVICE:
                    process_service( llu );
                    break;
                case rpc::message_info::MESSAGE_INTERNAL:
                    process_internal( llu );
                    break;
                /// answers; use id
                case rpc::message_info::MESSAGE_CLIENT_CALL:
                case rpc::message_info::MESSAGE_CLIENT_CALLBACK:
                    process_call( llu );
                    break;
                default:
                    process_invalid( llu );
                }
            }
        }

        static bool server_has_transformer( rpc::auth::init_capsule const &cap,
                                            unsigned transformer_type )
        {
            rpc::auth::init_protocol init;
            init.ParseFromString( cap.body( ) );
            return std::find( init.transform_supported( ).begin( ),
                      init.transform_supported( ).end( ),
                      transformer_type ) != init.transform_supported( ).end( );
        }

        void data_ready( )
        {
            stage_call_( );
        }

        void drop_service( const std::string &name )
        {
            client_->erase_rpc_handler( name );
        }

        void drop_all_services(  )
        {
            client_->erase_all_rpc_handlers( );
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
        impl_->conn_setup_->close( );
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

    void protocol_layer_c::on_init_error( const rpc::errors::container &error,
                                          const char *message )
    {
        impl_->callbacks_->on_init_error( error, message );
    }

    common::rpc_service_wrapper_sptr protocol_layer_c::get_service_by_name(
                                                        const std::string &name)
    {
        return impl_->get_service_by_name( name );
    }

    void protocol_layer_c::drop_service( const std::string &name )
    {
        impl_->drop_service( name );
    }

    void protocol_layer_c::drop_all_services(  )
    {
        impl_->drop_all_services( );
    }

    void protocol_layer_c::assign_lowlevel_factory( lowlevel_factory_type fac )
    {
        impl_->ll_factory_ = fac ? fac : impl_->create_default_factory( );
    }

}}
