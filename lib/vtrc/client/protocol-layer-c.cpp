#include <algorithm>
#include <iostream>

#include "vtrc-asio.h"
#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"

#include "vtrc-bind.h"
#include "vtrc-function.h"
#include "vtrc-chrono.h"

#include "vtrc/common/data-queue.h"
#include "vtrc/common/hash/iface.h"
#include "vtrc/common/transformer/iface.h"
#include "vtrc/common/call-context.h"
#include "vtrc/common/random-device.h"
#include "vtrc/common/protocol/accessor-iface.h"

#include "vtrc/common/lowlevel-protocol-iface.h"

#include "vtrc/common/transport/iface.h"
#include "vtrc/common/rpc-controller.h"

#include "vtrc/client/protocol-layer-c.h"

#include "vtrc/client/client.h"

#include "vtrc-auth.pb.h"
#include "vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace client {

    namespace gpb = google::protobuf;

    typedef vtrc::function<void (const rpc::errors::container &,
                                 const char *)> init_error_cb;

    typedef common::lowlevel::protocol_layer_iface
                              lowlevel_protocol_layer_iface;

    lowlevel_protocol_layer_iface *create_default_setup( client::base *c );

    namespace {

        typedef vtrc::shared_ptr<rpc::lowlevel_unit> lowlevel_unit_sptr;

        typedef vtrc::shared_ptr<gpb::Service> service_str;

        //namespace default_cypher = common::transformers::chacha;

        bool alone_callback(const lowlevel_unit_sptr &llu)
        {
            return llu->info( ).message_flags( ) &
                   rpc::message_info::FLAG_CALLBACK_ALONE;
        }

        void fill_error( lowlevel_unit_sptr &llu, unsigned code,
                         const char *message, bool fatal )
        {
            rpc::errors::container *err = llu->mutable_error( );
            err->set_code( code );
            err->set_additional( message );
            err->set_fatal( fatal );
        }

    }

    struct protocol_layer_c::impl: common::protocol_accessor {

        typedef impl this_type;
        typedef protocol_layer_c parent_type;

        common::connection_iface        *connection_;
        protocol_layer_c                *parent_;
        client::base                    *client_;
        protocol_signals                *callbacks_;

        bool                             closed_;
        lowlevel_protocol_layer_iface   *conn_setup_;
        lowlevel_factory_type            ll_factory_;

        impl( common::connection_iface *c, client::base *client,
              protocol_signals *cb )
            :connection_(c)
            ,parent_(VTRC_NULL)
            ,client_(client)
            ,callbacks_(cb)
            ,closed_(false)
            ,conn_setup_(VTRC_NULL)
        { }

        protocol_layer_c::lowlevel_factory_type create_default_factory( )
        {
            return vtrc::bind( create_default_setup, client_ );
        }

        //// ================ accessor =================

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
            parent_->set_ready( value );
            callbacks_->on_ready( value );
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

        static void closure_none( const VTRC_SYSTEM::error_code & )
        { }

        void init( )
        {
            conn_setup_ = ll_factory_( ); // create_default_setup( client_ );
            parent_->set_lowlevel( conn_setup_ );
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
            return client_->get_rpc_handler(name);
        }

        void process_call_( base_wptr client, lowlevel_unit_sptr llu )
        {
            base_sptr c(client.lock( ));
            if( !c ) {
                return;
            }
            parent_->make_local_call( llu );
        }

        void process_call( lowlevel_unit_sptr &llu )
        {
            if ( llu->id( ) != 0 ) {
                client_->execute( vtrc::bind( &this_type::process_call_, this,
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

        void process_answer( lowlevel_unit_sptr &llu )
        {
            parent_->push_rpc_message( llu->id( ), llu );
        }

        void process_insertion( lowlevel_unit_sptr &llu )
        {
            if( 0 == parent_->push_rpc_message( llu->target_id( ), llu ) ) {
                if( alone_callback( llu ) ) {
                    process_call( llu );
                } else {
                    if( llu->opt( ).wait( ) ) {
                        llu->clear_request( );
                        llu->clear_response( );
                        llu->clear_call( );
                        llu->clear_opt( );
                        fill_error( llu, rpc::errors::ERR_CONTEXT,
                                    "Context was not found.", false );
                        parent_->call_rpc_method( *llu );
                    }
                }
            }
        }

        void process_invalid( lowlevel_unit_sptr &llu )
        {

        }

        void on_rpc_process( )
        {
            while( !parent_->message_queue_empty( ) ) {

                lowlevel_unit_sptr llu(vtrc::make_shared<lowlevel_unit_type>());

                bool check = parent_->parse_and_pop( *llu );

                if( !check ) {

                    llu->Clear( );
                    fill_error( llu, rpc::errors::ERR_PROTOCOL,
                                "Bad message was received", true );

                    parent_->push_rpc_message_all( llu );
                    connection_->close( );

                    return;
                }

                switch( llu->info( ).message_type( ) ) {

                /// SERVER_CALL = request; do not use id
                case rpc::message_info::MESSAGE_SERVER_CALL:
                    process_call( llu );
                    break;

                /// SERVER_CALLBACK = request; use target_id
                case rpc::message_info::MESSAGE_SERVER_CALLBACK:
                    process_insertion( llu );
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
                    process_answer( llu );
                    break;
                default:
                    process_invalid( llu );
                }
            }
        }

        void message_ready( )
        {
            on_rpc_process( );
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

    protocol_layer_c::protocol_layer_c( common::connection_iface *connection,
                                        vtrc::client::base *client ,
                                        protocol_signals *callbacks)
        :common::protocol_layer(connection, true)
        ,impl_(new impl(connection, client, callbacks))
    {
        lowlevel_factory_type fac = client->lowlevel_protocol_factory( );

        impl_->parent_     = this;
        impl_->ll_factory_ = fac ? fac : impl_->create_default_factory( );
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
        if( impl_->conn_setup_ ) {
            impl_->conn_setup_->close( );
        }
        cancel_all_slots( );
        impl_->callbacks_->on_disconnect( );
    }

    const std::string &protocol_layer_c::client_id( ) const
    {
        return impl_->client_id( );
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

}}
