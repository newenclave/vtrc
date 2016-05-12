#include <algorithm>
#include "boost/asio.hpp"

#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-bind.h"
#include "vtrc-function.h"

#include "vtrc-protocol-layer-s.h"

#include "vtrc-common/vtrc-monotonic-timer.h"
#include "vtrc-common/vtrc-data-queue.h"

#include "vtrc-transport-iface.h"

#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-rpc-controller.h"
#include "vtrc-common/vtrc-call-context.h"

#include "vtrc-application.h"

#include "vtrc-errors.pb.h"
#include "vtrc-rpc-lowlevel.pb.h"

#include "vtrc-chrono.h"
#include "vtrc-atomic.h"
#include "vtrc-common/vtrc-protocol-accessor-iface.h"
#include "vtrc-common/vtrc-lowlevel-protocol-iface.h"

namespace vtrc { namespace server {

    namespace gpb   = google::protobuf;
    namespace bsys  = boost::system;
    namespace basio = boost::asio;

    typedef common::lowlevel_protocol_layer_iface connection_setup_iface;
    connection_setup_iface *create_default_setup( application &a,
                                            const rpc::session_options &opts );

    typedef connection_setup_iface *connection_setup_ptr;

    namespace {

        typedef std::map <
            std::string,
            common::rpc_service_wrapper_sptr
        > service_map;

        typedef vtrc::function<void (void)> stage_function_type;

        typedef rpc::lowlevel_unit                   lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        void def_cb( const boost::system::error_code & )
        { }
    }

    namespace data_queue = common::data_queue;
    typedef common::protocol_accessor paccessor;

    struct protocol_layer_s::impl: common::protocol_accessor {

        typedef impl             this_type;
        typedef protocol_layer_s parent_type;

        application                     &app_;
        common::transport_iface         *connection_;
        common::connection_iface_wptr    keeper_;
        protocol_layer_s                *parent_;
        bool                             ready_;
        bool                             closed_;

        service_map                      services_;
        shared_mutex                     services_lock_;

        std::string                      client_id_;

        vtrc::atomic<unsigned>           current_calls_;

        stage_function_type              stage_call_;

        connection_setup_ptr             conn_setup_;

        impl( application &a, common::transport_iface *c )
            :app_(a)
            ,connection_(c)
            ,keeper_(c->weak_from_this( ))
            ,ready_(false)
            ,closed_(false)
            ,current_calls_(0)
        { }

        void call_setup_function( )
        {
            conn_setup_->do_handshake( );
        }

        // accessor ===================================
        void write( const std::string &data,
                    common::system_closure_type cb,
                    bool on_send_success )
        {
            connection_->write( data.c_str( ), data.size( ),
                                cb, on_send_success );
        }

        void set_client_id( const std::string &id )
        {
            client_id_.assign( id );
        }

        void ready( bool value )
        {
            stage_call_ = value
                        ? vtrc::bind( &this_type::on_rcp_call_ready, this )
                        : vtrc::bind( &this_type::call_setup_function, this );
            parent_->set_ready( value );
        }

        void close( )
        {
            closed_ = true;
            connection_->close( );
        }

        common::connection_iface *connection( )
        {
            return connection_;
        }

        // accessor ===================================

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

        void drop_service( const std::string &name )
        {
            unique_shared_lock _(services_lock_);
            services_.erase( name );
        }

        void drop_all_services(  )
        {
            unique_shared_lock _(services_lock_);
            services_.clear( );
        }

        const std::string &client_id( ) const
        {
            return client_id_;
        }

        void send_proto_message( const gpb::MessageLite &mess,
                                 common::system_closure_type closure,
                                 bool on_send)
        {
            std::string s( mess.SerializeAsString( ) );
            connection_->write( s.c_str( ), s.size( ), closure, on_send );
        }

        void send_and_close( const gpb::MessageLite &mess )
        {
            DEBUG_LINE(connection_);

            send_proto_message( mess,
                                vtrc::bind( &this_type::close_client, this,
                                             vtrc::placeholders::_1,
                                             connection_->weak_from_this( ) ),
                                true );
        }

        void close_client( const bsys::error_code &      /*err */,
                           common::connection_iface_wptr &inst )
        {
            common::connection_iface_sptr lcked( inst.lock( ) );
            if( lcked ) {
                lcked->close( );
            }
        }

        bool get_pop_message( gpb::Message &capsule )
        {
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
                vtrc::bind( &this_type::call_done, this,
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

        void init( )
        {
            conn_setup_ =
                    create_default_setup( app_, parent_->session_options( ) );
            stage_call_ = vtrc::bind( &this_type::call_setup_function, this );
            parent_->set_lowlevel( conn_setup_ );
            conn_setup_->init( this, def_cb );
        }

        void init_success( common::system_closure_type clos )
        {
            conn_setup_ =
                    create_default_setup( app_, parent_->session_options( ) );
            stage_call_ = vtrc::bind( &this_type::call_setup_function, this );
            parent_->set_lowlevel( conn_setup_ );
            conn_setup_->init( this, clos );
        }

        void data_ready( )
        {
            common::connection_iface_sptr lckd( keeper_.lock( ) );
            if( !lckd ) {
                return;
            }
            stage_call_( );
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

    void protocol_layer_s::drop_service( const std::string &name )
    {
        impl_->drop_service( name );
    }

    void protocol_layer_s::drop_all_services(  )
    {
        impl_->drop_all_services( );
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

