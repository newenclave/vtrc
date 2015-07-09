#include <iostream>

#include "stress-application.h"

#include "vtrc-rpc-lowlevel.pb.h"

#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-delayed-call.h"

#include "vtrc-common/vtrc-connection-list.h"

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-tcp.h"
#include "vtrc-server/vtrc-listener-local.h"
#include "vtrc-server/vtrc-listener.h"

#include "stress-service-impl.h"

#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"

#include "vtrc-bind.h"
#include "vtrc-ref.h"
#include "vtrc-mutex.h"
#include "vtrc-atomic.h"
#include "vtrc-thread.h"

#include "boost/asio.hpp"
#include "boost/asio/error.hpp"

#include "google/protobuf/service.h"

#include "vtrc-common/vtrc-delayed-call.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-common/vtrc-closure.h"

namespace stress {

namespace {

    void precall_closure( vtrc::common::connection_iface &c,
                          const google::protobuf::MethodDescriptor *m,
                          vtrc::rpc::lowlevel_unit & )
    {
        std::cout << "Precall " << m->full_name( ) << "\n";
        c.close( );
    }

    void postcall_closure( vtrc::common::connection_iface &,
                           vtrc::rpc::lowlevel_unit & )
    {
        std::cout << "Postcall " << "\n";
    }

}

    using namespace vtrc;
    namespace po = boost::program_options;
    namespace gpb = google::protobuf;
    namespace basio = boost::asio;
    namespace bsys  = boost::system;

    typedef vtrc::common::delayed_call            delayed_call;
    typedef vtrc::shared_ptr<delayed_call>        delayed_call_sptr;

    typedef std::pair<
            delayed_call_sptr,
            application::timer_callback > call_cb_pair;

    typedef std::map<unsigned, call_cb_pair> delayed_map;

    namespace {

        server::listener_sptr create_from_string( const std::string &name,
                                  server::application &app,
                                  const rpc::session_options &opts,
                                  bool tcp_nodelay)
        {
            /// result endpoint
            server::listener_sptr result;

            std::vector<std::string> params;

            size_t delim_pos = name.find_last_of( ':' );
            if( delim_pos == std::string::npos ) {

                /// local: <localname>
                params.push_back( name );

            } else {

                /// tcp: <addres>:<port>
                params.push_back( std::string( name.begin( ),
                                               name.begin( ) + delim_pos ) );

                params.push_back( std::string( name.begin( ) + delim_pos + 1,
                                               name.end( ) ) );
            }

            if( params.size( ) == 1 ) {         /// local endpoint
#ifndef _WIN32
                ::unlink( params[0].c_str( ) ); /// unlink old file socket
#endif
                result = server::listeners::local::create(app, opts, params[0]);
//                result->set_precall( &precall_closure );
//                result->set_postcall( &postcall_closure );

            } else if( params.size( ) == 2 ) {  /// TCP

                result = server::listeners::tcp::create( app, opts,
                                params[0],
                                boost::lexical_cast<unsigned short>(params[1]),
                                tcp_nodelay);

            }
            return result;
        }

        class app_impl: public server::application {

        public:

            stress::application *parent_app_;

            app_impl( common::pool_pair &pp )
                :server::application(pp)
            { }

            void configure_session( common::connection_iface* connection,
                                            rpc::session_options &res )
            {
                res.set_max_active_calls  ( 5 );
                res.set_max_message_length( 65536 );
                res.set_max_stack_size    ( 64 );
                res.set_read_buffer_size  ( 4096 );
            }

            vtrc::shared_ptr<common::rpc_service_wrapper>
                     get_service_by_name( common::connection_iface* conn,
                                          const std::string &service_name )
            {
                if( service_name == stress::service_name( ) ) {

                    vtrc::shared_ptr<gpb::Service> stress_serv
                            ( stress::create_service( conn, *parent_app_ ) );

                    return vtrc::shared_ptr<common::rpc_service_wrapper>(
                                vtrc::make_shared<common::rpc_service_wrapper>(
                                                                stress_serv ) );
                }
                return vtrc::shared_ptr<common::rpc_service_wrapper>( );
            }

        };

    }

    struct application::impl {

        common::pool_pair                   pp_;
        app_impl                            app_;
        std::vector<server::listener_sptr>  listeners_;

        vtrc::atomic<size_t>                counter_;
        common::delayed_call                retry_timer_;

        unsigned                            accept_errors_;
        unsigned                            max_clients_;
        vtrc::mutex                         counter_lock_;

        vtrc::atomic<unsigned>              next_id_;
        common::thread_pool                 timer_pool_;
        delayed_map                         map_;
        vtrc::shared_mutex                  map_lock_;

        impl( unsigned io_threads )
            :pp_(io_threads)
            ,app_(pp_)
            ,counter_(0)
            ,retry_timer_(pp_.get_io_service( ))
            ,accept_errors_(0)
            ,max_clients_(1000)
            ,next_id_(0)
            ,timer_pool_(1)
        { }

        impl( unsigned io_threads, unsigned rpc_threads )
            :pp_(io_threads, rpc_threads)
            ,app_(pp_)
            ,counter_(0)
            ,retry_timer_(pp_.get_io_service( ))
            ,max_clients_(1000)
            ,next_id_(0)
            ,timer_pool_(1)
        { }

        unsigned next_id( )
        {
            return ++next_id_;
        }

        void timer_handler( const bsys::error_code &err,
                            unsigned id,  unsigned microsec,
                            call_cb_pair tc )
        {
            unsigned err_code = err.value( );
            if( tc.second( id, err_code )) {
                start_timer( tc, id, microsec );
            }
        }

        typedef common::timer::monotonic_traits::microseconds microseconds;
        typedef common::timer::monotonic_traits::milliseconds milliseconds;

        void start_timer( call_cb_pair &tc, unsigned id,  unsigned microsec )
        {
            tc.first->call_from_now(
                        vtrc::bind( &impl::timer_handler, this,
                                    vtrc::placeholders::error,
                                    id, microsec, tc ),
                        microseconds( microsec ) );
        }

        void on_new_connection( server::listener *l,
                                const common::connection_iface *c )
        {
            std::cout << "New connection: "
                      << "\n\tep:     " << l->name( )
                      << "\n\tclient: " << c->name( )
                      << "\n\ttotal:  " << ++counter_
                      << "\n"
                        ;
        }

        void on_stop_connection( server::listener *l,
                                 const common::connection_iface *c )
        {
            std::cout << "Close connection: "
                      << c->name( )
                      << "; count: " << --counter_
                      << "\n";
        }

        void start_retry_accept( server::listener_sptr l, unsigned rto )
        {
            retry_timer_.call_from_now(
                vtrc::bind( &impl::retry_timer_handler, this,
                            l, rto, vtrc::placeholders::_1 ),
                            milliseconds( rto ));
        }

        void retry_timer_handler( server::listener_sptr l, unsigned retry_to,
                                  const boost::system::error_code &code)
        {
            if( !code ) {
                std::cout << "Restarting " << l->name( ) << "...";
                try {
                    l->start( );
                    std::cout << "Ok;\n";
                } catch( const std::exception &ex ) {
                    std::cout << "failed; " << ex.what( )
                              << "; Retrying...\n";
                    start_retry_accept( l, retry_to );
                };
            }
        }

        void on_accept_failed( server::listener *l,
                               unsigned retry_to,
                               const boost::system::error_code &code )
        {
            std::cout << "Accept failed at " << l->name( )
                      << " due to '" << code.message( ) << "'\n";
            start_retry_accept( l->shared_from_this( ), retry_to );
        }


        rpc::session_options options( const po::variables_map &params )
        {
            using common::defaults::session_options;
            rpc::session_options opts( session_options( ) );

            if( params.count( "message-size" ) ) {
                opts.set_max_message_length(
                            params["message-size"].as<unsigned>( ));
            }

            if( params.count( "stack-size" ) ) {
                opts.set_max_stack_size(
                            params["stack-size"].as<unsigned>( ));
            }

            if( params.count( "read-size" ) ) {
                opts.set_read_buffer_size(
                            params["read-size"].as<unsigned>( ));
            }

            if( params.count( "max-calls" ) ) {
                opts.set_max_active_calls(
                            params["max-calls"].as<unsigned>( ));
            }

            return opts;
        }

        void run( const po::variables_map &params )
        {

            typedef std::vector<std::string> string_vector;
            typedef string_vector::const_iterator vec_citer;

            string_vector ser = params["server"].as<string_vector>( );

            bool use_only_pool = !!params.count( "only-pool" );
            bool tcp_nodelay   = !!params.count( "tcp-nodelay" );

            rpc::session_options opts( options( params ) );

            unsigned retry_to = (params.count( "accept-retry" ) != 0)
                    ? params["accept-retry"].as<unsigned>( )
                    : 1000;

            if( params.count( "max-clients" ) ) {
                max_clients_ = params["max-clients"].as<unsigned>( );
            }

            for( vec_citer b(ser.begin( )), e(ser.end( )); b != e; ++b ) {

                std::cout << "Starting listener at '" <<  *b << "'...";
                attach_start_listener( retry_to,
                    create_from_string( *b, app_, opts, tcp_nodelay) );
                std::cout << "Ok\n";

            }

            if( use_only_pool ) {
                pp_.get_io_pool( ).attach( );
            } else {
                pp_.get_rpc_pool( ).attach( );
            }
            pp_.join_all( );
        }

        void attach_start_listener( unsigned retry_to,
                                    vtrc::server::listener_sptr listen )
        {
            listen->on_new_connection_connect(
                   vtrc::bind( &impl::on_new_connection, this,
                               listen.get( ), vtrc::placeholders::_1 ));

            listen->on_stop_connection_connect(
                   vtrc::bind( &impl::on_stop_connection, this,
                               listen.get( ), vtrc::placeholders::_1 ));

            listen->on_accept_failed_connect(
                   vtrc::bind( &impl::on_accept_failed, this,
                               listen.get( ), retry_to,
                               vtrc::placeholders::_1 ) );

            listeners_.push_back( listen );

            listen->start( );

        }

        void stop( )
        {
            std::cout << "Stopping server ...";

            timer_pool_.stop( );
            timer_pool_.join_all( );

            typedef std::vector<server::listener_sptr>::const_iterator citer;
            for( citer b(listeners_.begin( )), e(listeners_.end( )); b!=e; ++b){
                (*b)->stop( );
            }
            retry_timer_.cancel( );
            pp_.stop_all( );
            std::cout << "Ok\n";
        }

    };

    application::application( unsigned io_threads )
        :impl_(new impl(io_threads))
    {
        impl_->app_.parent_app_ = this;
    }

    application::application( unsigned io_threads, unsigned rpc_threads )
        :impl_(new impl(io_threads, rpc_threads))
    {
        impl_->app_.parent_app_ = this;
    }

    application::~application( )
    {
        delete impl_;
    }

    server::application &application::get_application( )
    {
        return impl_->app_;
    }

    const server::application &application::get_application( ) const
    {
        return impl_->app_;
    }

    void application::run( const po::variables_map &params )
    {
        impl_->run( params );
    }

    void application::stop( )
    {
        impl_->stop( );
    }

    unsigned application::add_timer_event( unsigned microseconds,
                                           timer_callback cb )
    {
        unsigned id = impl_->next_id( );
        delayed_call_sptr md(
                    new delayed_call( impl_->timer_pool_.get_io_service( )) );
        call_cb_pair p( std::make_pair( md, cb ) );
        impl_->start_timer( p, id, microseconds );

        vtrc::unique_shared_lock usl(impl_->map_lock_);
        impl_->map_.insert( std::make_pair( id, p ) );

        std::cout << "Add timer " << id << " success\n";

        return id;
    }

    void application::del_timer_event( unsigned id )
    {
        vtrc::shared_lock usl(impl_->map_lock_);
        delayed_map::iterator f( impl_->map_.find( id ));
        if( f != impl_->map_.end( ) ) {
            f->second.first->cancel( );
            impl_->map_.erase( f );
        }
    }

}

