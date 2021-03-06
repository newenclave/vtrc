#include <algorithm>
#include "vtrc-asio.h"

#include "vtrc-bind.h"
#include "vtrc-function.h"
#include "vtrc-ref.h"

#include "vtrc/server/protocol-layer-s.h"

#include "vtrc/common/monotonic-timer.h"
#include "vtrc/common/data-queue.h"

#include "vtrc/common/transport/iface.h"

#include "vtrc/common/rpc-service-wrapper.h"
#include "vtrc/common/exception.h"
#include "vtrc/common/rpc-controller.h"
#include "vtrc/common/call-context.h"

#include "vtrc/server/application.h"

#include "vtrc/common/protocol/vtrc-errors.pb.h"
#include "vtrc/common/protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-chrono.h"
#include "vtrc-mutex.h"
#include "vtrc-atomic.h"

#include "vtrc/common/protocol/accessor-iface.h"
#include "vtrc/common/lowlevel-protocol-iface.h"

namespace vtrc { namespace server {

    namespace gpb   = google::protobuf;
    namespace bsys  = VTRC_SYSTEM;
    namespace basio = VTRC_ASIO;

    typedef common::lowlevel::protocol_layer_iface connection_setup_iface;
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

        void def_cb( const VTRC_SYSTEM::error_code & )
        { }

        /// TODO: Copy-Paste
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

    namespace data_queue = common::data_queue;
    typedef common::protocol_accessor paccessor;
    typedef protocol_layer_s::lowlevel_factory_type lowlevel_factory_type;


    struct protocol_layer_s::impl: common::protocol_accessor {

        typedef impl             this_type;
        typedef protocol_layer_s parent_type;
        typedef vtrc::lock_guard<vtrc::mutex> locker_type;

        application                     &app_;
        common::connection_iface        *connection_;
        common::connection_iface_wptr    keeper_;
        protocol_layer_s                *parent_;
        bool                             ready_;
        bool                             closed_;

        service_map                      services_;
        vtrc::mutex                      services_lock_;

        std::string                      client_id_;

        vtrc::atomic<unsigned>           current_calls_;

        connection_setup_ptr             conn_setup_;
        lowlevel_factory_type            ll_factory_;

        impl( application &a, common::connection_iface *c )
            :app_(a)
            ,connection_(c)
            ,keeper_(c->weak_from_this( ))
            ,parent_(VTRC_NULL)
            ,ready_(false)
            ,closed_(false)
            ,current_calls_(0)
            ,conn_setup_(VTRC_NULL)
            ,ll_factory_(&common::lowlevel::dummy::create)
        { }

        protocol_layer_s::lowlevel_factory_type create_default_factory( )
        {
            return vtrc::bind( create_default_setup, vtrc::ref(app_),
                               vtrc::ref(parent_->session_options( )) );
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
            parent_->set_ready( value );
        }

        void close( )
        {
            closed_ = true;
            if( conn_setup_ ) {
                conn_setup_->close( );
            }
        }

        common::connection_iface *connection( )
        {
            return connection_;
        }

        // accessor ===================================

        common::rpc_service_wrapper_sptr get_service( const std::string &name )
        {
            locker_type lk( services_lock_ );
            common::rpc_service_wrapper_sptr result;
            service_map::iterator f( services_.find( name ) );

            if( f != services_.end( ) ) {
                result = f->second;
            } else {
                result = app_.get_service_by_name( connection_, name );
                if( result ) {
                    services_.insert( std::make_pair( name, result ) );
                }
            }

            return result;
        }

        void drop_service( const std::string &name )
        {
            locker_type _(services_lock_);
            services_.erase( name );
        }

        void drop_all_services(  )
        {
            locker_type _(services_lock_);
            services_.clear( );
        }

        const std::string &client_id( ) const
        {
            return client_id_;
        }

        bool get_pop_message( rpc::lowlevel_unit &capsule )
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
                app_.execute( vtrc::bind( &this_type::push_call, this, llu,
                                          connection_->shared_from_this( )));
            } else {
                send_busy( *llu );
                --current_calls_;
            }
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
                    process_answer( llu );
                    break;
                default:
                    break;
                }
            }
        }

        void init( )
        {
            init_success( &def_cb );
        }

        void init_success( common::system_closure_type clos )
        {
            conn_setup_ = ll_factory_( );
            parent_->set_lowlevel( conn_setup_ );
            conn_setup_->init( this, clos );
        }

        void message_ready( )
        {
            data_ready_( );
        }

        void data_ready_( )
        {
            common::connection_iface_sptr lckd( keeper_.lock( ) );
            if( !lckd ) {
                return;
            }
            on_rcp_call_ready( );
        }

        void data_ready( )
        { }

    };

    protocol_layer_s::protocol_layer_s( application &a,
                                        common::connection_iface *connection,
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
        cancel_all_slots( );
        impl_->close( );
    }

    void protocol_layer_s::drop_service( const std::string &name )
    {
        impl_->drop_service( name );
    }

    void protocol_layer_s::drop_all_services(  )
    {
        impl_->drop_all_services( );
    }

    void protocol_layer_s::assign_lowlevel_factory( lowlevel_factory_type fac )
    {
        impl_->ll_factory_ = fac ? fac : impl_->create_default_factory( );
    }

    const std::string &protocol_layer_s::client_id( ) const
    {
        return impl_->client_id( );
    }

    common::rpc_service_wrapper_sptr protocol_layer_s::get_service_by_name(
                                                        const std::string &name)
    {
        return impl_->get_service(name);
    }

    namespace protocol {
        common::protocol_iface *create( application &app,
                                        common::connection_iface_sptr conn,
                                        const rpc::session_options &opts )
        {
            protocol_layer_s *n = new protocol_layer_s( app, conn.get( ),
                                                        opts );
            return n;
        }

        common::protocol_iface *create( application &app,
                                        common::connection_iface_sptr conn )
        {
            rpc::session_options opts = common::defaults::session_options( );
            return create( app, conn, opts );
        }
    }

}}

