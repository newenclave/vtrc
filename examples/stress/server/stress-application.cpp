#include "stress-application.h"

#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-connection-iface.h"

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-tcp.h"
#include "vtrc-server/vtrc-listener-local.h"
#include "vtrc-server/vtrc-listener.h"

#include "stress-service-impl.h"

#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"

#include "vtrc-bind.h"
#include "vtrc-ref.h"

namespace stress {

    using namespace vtrc;
    namespace po = boost::program_options;

    namespace {

        server::listener_sptr create_from_string( const std::string &name,
                                  server::application &app,
                                  const server::listener_options &opts,
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
                result = server::listeners::local::create( app, def_opts,
                                                           params[0] );

            } else if( params.size( ) == 2 ) {  /// TCP

                result = server::listeners::tcp::create( app, def_opts,
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
                    google::protobuf::Service *stress_serv =
                            stress::create_service( conn );
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

        impl( unsigned io_threads )
            :pp_(io_threads)
            ,app_(pp_)
        { }

        impl( unsigned io_threads, unsigned rpc_threads )
            :pp_(io_threads, rpc_threads)
            ,app_(pp_)
        { }

        void on_new_connection( server::listener_sptr l,
                                const common::connection_iface *c )
        {
            std::cout << "New connection: "
                      << "\n\tep:     " << l->name( )
                      << "\n\tclient: " << c->name( )
                      << "\n"
                        ;
        }

        void on_stop_connection( server::listener_sptr l,
                                 const common::connection_iface *c )
        {
            std::cout << "Close connection: "
                      << c->name( ) << "\n";
        }

        void run( const po::variables_map &params )
        {
            typedef std::vector<std::string> string_vector;
            typedef string_vector::const_iterator citer;

            string_vector ser = params["server"].as<string_vector>( );

            bool use_only_pool = !!params.count( "only-pool" );
            bool tcp_nodelay   = !!params.count( "tcp-nodelay" );

            using server::listeners::default_options;

            server::listener_options opts( default_options( ) );

            for( citer b(ser.begin( )), e(ser.end( )); b != e; ++b) {

                std::cout << "Starting listener at '" <<  *b << "'...";
                attach_start_listener( create_from_string( *b, app_, opts,
                                                           tcp_nodelay) );
                std::cout << "Ok\n";

            }

            if( use_only_pool ) {
                pp_.get_io_pool( ).attach( );
            } else {
                pp_.get_rpc_pool( ).attach( );
            }
            pp_.join_all( );
        }

        void attach_start_listener( vtrc::server::listener_sptr listen )
        {
            listen->get_on_new_connection( ).connect(
                   vtrc::bind( &impl::on_new_connection, this, listen, _1 ));

            listen->get_on_stop_connection( ).connect(
                   vtrc::bind( &impl::on_stop_connection, this, listen, _1 ));

            listeners_.push_back( listen );
            listen->start( );
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

}

