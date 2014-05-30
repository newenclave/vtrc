#include "stress-application.h"

#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-delayed-call.h"

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

#include "boost/asio/error.hpp"

namespace stress {

    using namespace vtrc;
    namespace po = boost::program_options;
    namespace gpb = google::protobuf;

    namespace {

        server::listener_sptr create_from_string( const std::string &name,
                                  server::application &app,
                                  const vtrc_rpc::session_options &opts,
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
            app_impl( common::pool_pair &pp )
                :server::application(pp)
            { }

            vtrc::shared_ptr<common::rpc_service_wrapper>
                     get_service_by_name( common::connection_iface* conn,
                                          const std::string &service_name )
            {
                if( service_name == stress::service_name( ) ) {

                    gpb::Service *stress_serv = stress::create_service( conn );

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

        impl( unsigned io_threads )
            :pp_(io_threads)
            ,app_(pp_)
            ,counter_(0)
            ,retry_timer_(pp_.get_io_service( ))
            ,accept_errors_(0)
        { }

        impl( unsigned io_threads, unsigned rpc_threads )
            :pp_(io_threads, rpc_threads)
            ,app_(pp_)
            ,counter_(0)
            ,retry_timer_(pp_.get_io_service( ))
        { }

        void on_new_connection( server::listener_sptr l,
                                const common::connection_iface *c )
        {
            //vtrc::unique_lock<vtrc::mutex> lock(counter_lock_);
            std::cout << "New connection: "
                      << "\n\tep:     " << l->name( )
                      << "\n\tclient: " << c->name( )
                      << "\n\ttotal:  " << ++counter_
                      << "\n"
                        ;
        }

        void start_retry_accept( server::listener_sptr l, unsigned rto )
        {
            retry_timer_.call_from_now(
                vtrc::bind( &impl::retry_timer_handler, this, l, rto, _1 ),
                            common::timer::milliseconds( rto ));
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

        void on_accept_failed( server::listener_sptr l,
                               unsigned retry_to,
                               const boost::system::error_code &code )
        {
            std::cout << "Accept failed at " << l->name( )
                      << " due to '" << code.message( ) << "'\n";
            start_retry_accept( l, retry_to );
        }

        void on_stop_connection( server::listener_sptr l,
                                 const common::connection_iface *c )
        {
            //vtrc::unique_lock<vtrc::mutex> lock(counter_lock_);
            std::cout << "Close connection: "
                      << c->name( )
                      << "; count: " << --counter_
                      << "\n";
        }

        vtrc_rpc::session_options options( const po::variables_map &params )
        {
            using server::listeners::default_options;
            vtrc_rpc::session_options opts( default_options( ) );

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

            return opts;
        }

        void run( const po::variables_map &params )
        {
            typedef std::vector<std::string> string_vector;
            typedef string_vector::const_iterator vec_citer;

            string_vector ser = params["server"].as<string_vector>( );

            bool use_only_pool = !!params.count( "only-pool" );
            bool tcp_nodelay   = !!params.count( "tcp-nodelay" );

            using server::listeners::default_options;

            vtrc_rpc::session_options opts( options( params ) );

            unsigned retry_to = (params.count( "accept-retry" ) != 0)
                    ? params["accept-retry"].as<unsigned>( )
                    : 1000;

            for( vec_citer b(ser.begin( )), e(ser.end( )); b != e; ++b) {

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
            listen->get_on_new_connection( ).connect(
                   vtrc::bind( &impl::on_new_connection, this, listen, _1 ));

            listen->get_on_stop_connection( ).connect(
                   vtrc::bind( &impl::on_stop_connection, this, listen, _1 ));

            listen->get_on_accept_failed( ).connect(
                   vtrc::bind( &impl::on_accept_failed, this,
                               listen, retry_to, _1 ));

            listeners_.push_back( listen );
            listen->start( );
        }

        void stop( )
        {
            typedef std::vector<server::listener_sptr>::const_iterator citer;
            for( citer b(listeners_.begin( )), e(listeners_.end( )); b!=e; ++b){
                (*b)->stop( );
            }
            retry_timer_.cancel( );
            pp_.stop_all( );
            pp_.join_all( );
        }

    };

    application::application( unsigned io_threads )
        :impl_(new impl(io_threads))
    { }

    application::application( unsigned io_threads, unsigned rpc_threads )
        :impl_(new impl(io_threads, rpc_threads))
    { }

    application::~application()
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

}

