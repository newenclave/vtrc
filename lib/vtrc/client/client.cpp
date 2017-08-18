#include <map>
#include <vector>
#include <string>

#include "google/protobuf/descriptor.h"

#include "vtrc/client/client.h"

#include "vtrc/client/tcp.h"
#include "vtrc/client/ssl.h"
#include "vtrc/client/unix/local.h"
#include "vtrc/client/win/local.h"
#include "vtrc/client/posix-stream.h"

#include "vtrc/client/rpc-channel-c.h"
#include "vtrc/client/protocol-layer-c.h"

#include "vtrc-asio.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"
#include "vtrc-condition-variable.h"
#include "vtrc-chrono.h"
#include "vtrc-cxx11.h"

#include "vtrc/common/exception.h"
#include "vtrc/common/pool-pair.h"

#include "vtrc-errors.pb.h"

#include "vtrc/common/observer/scoped-subscription.h"

namespace vtrc { namespace client {

    namespace basio = VTRC_ASIO;
    namespace bsys  = VTRC_SYSTEM;
    namespace gpb   = google::protobuf;
    namespace bs2   = vtrc::common::observer;
    namespace ph    = vtrc::placeholders;

    namespace {

        typedef vtrc::weak_ptr<gpb::Service>    service_wptr;
        typedef vtrc::shared_ptr<gpb::Service>  service_sptr;

        typedef vtrc::shared_ptr<gpb::RpcChannel>  channel_sptr;

        typedef vtrc_client::service_wrapper_type service_wrapper_type;
        typedef vtrc_client::service_wrapper_wptr service_wrapper_wptr;
        typedef vtrc_client::service_wrapper_sptr service_wrapper_sptr;

        typedef std::map<std::string, service_wptr> native_service_weak_map;
        typedef std::map<std::string, service_wrapper_wptr> service_weak_map;
        typedef std::map<std::string, service_wrapper_sptr> service_shared_map;

        typedef common::protocol_iface::call_type       proto_call_type;
        typedef common::protocol_iface::executor_type   executor_type;
    }

    struct vtrc_client::impl: public protocol_signals {

        typedef impl this_type;

        vtrc_client                    *parent_;

        native_service_weak_map         weak_native_services_;
        service_weak_map                weak_services_;
        service_shared_map              hold_services_;
        //vtrc::mutex                     services_lock_;

        std::string                     session_key_;
        std::string                     session_id_;
        bool                            key_set_;
        //vtrc::mutex                     session_info_lock_;

        impl( )
            :parent_(VTRC_NULL)
            ,key_set_(false)
        { }


        bool ready( ) const
        {
            return  parent_->connection( )
                  ? parent_->connection( )->get_protocol( ).ready( )
                  : false;
        }

        /// ============= signals =============== /////
        void on_init_error(const rpc::errors::container &err,
                                   const char *message)
        {
            parent_->get_on_init_error( )( err, message );
        }

        void on_connect( )
        {
            parent_->get_on_connect( )( );
        }

        void on_disconnect( )
        {
            parent_->get_on_disconnect( )( );
        }

        void on_ready( bool /*value*/ )
        {
            parent_->get_on_ready( )( );
        }
        /// ====================================== /////

        const common::call_context *get_call_context( ) const
        {
            return common::call_context::get( );
        }

        void set_session_key( const std::string &id, const std::string &key )
        {
            parent_->set_session_id( id );
            set_session_key( key );
        }

        void set_session_key( const std::string &value )
        {
            parent_->env( ).set( "session_key", value );
        }

        std::string get_session_key(  ) const
        {
            return parent_->env( ).get( "session_key" );
        }

        void  reset_session_key( )
        {
            parent_->env( ).remove( "session_key" );
        }

        void  reset_session_info( )
        {
            parent_->reset_session_id( );
            reset_session_key( );
        }


        template<typename ClientType>
        vtrc::shared_ptr<ClientType> create_client(  )
        {
            vtrc::shared_ptr<ClientType> c(
                        ClientType::create( parent_, this ));

            parent_->reset_connection( c );
            return c;
        }

        vtrc::shared_ptr<client_tcp> create_client_tcp( bool tcp_nodelay )
        {
            vtrc::shared_ptr<client_tcp> new_client_inst
                    (client_tcp::create( parent_, this, tcp_nodelay ));

            parent_->reset_connection( new_client_inst );

            return new_client_inst;
        }

#if VTRC_OPENSSL_ENABLED

        vtrc::shared_ptr<client_ssl> create_client_ssl(
                        const std::string &verify_file, bool tcp_nodelay )
        {
            vtrc::shared_ptr<client_ssl> new_client_inst
                    (client_ssl::create( parent_, this,
                                         verify_file, tcp_nodelay ));

            parent_->reset_connection( new_client_inst );

            return new_client_inst;
        }

        void connect_ssl( const std::string &address,
                          unsigned short service,
                          const std::string &verfile,
                          verify_callback_type cb,
                          bool tcp_nodelay )
        {
            vtrc::shared_ptr<client_ssl>
                            new_client(create_client_ssl(verfile, tcp_nodelay));

            new_client->set_verify_callback( cb );
            connect_impl( vtrc::bind( &client_ssl::connect, new_client,
                                       address, service) );
            if( tcp_nodelay ) {
                new_client->set_no_delay( true );
            }
        }

        void connect_ssl( const std::string &address,
                          unsigned short service,
                          const std::string &verfile,
                          bool tcp_nodelay )
        {
            vtrc::shared_ptr<client_ssl>
                            new_client(create_client_ssl(verfile, tcp_nodelay));

            connect_impl( vtrc::bind( &client_ssl::connect, new_client,
                                       address, service) );
            if( tcp_nodelay ) {
                new_client->set_no_delay( true );
            }
        }

        void async_connect_ssl( const std::string &address,
                                unsigned short     service,
                                common::system_closure_type closure,
                                const std::string &verfil,
                                verify_callback_type cb,
                                bool tcp_nodelay )
        {
            vtrc::shared_ptr<client_ssl>
                          new_client( create_client_ssl(verfil, tcp_nodelay ) );

            new_client->set_verify_callback( cb );
            new_client->async_connect( address, service,
                    vtrc::bind( &this_type::async_connect_success, this,
                                ph::error, closure ));
        }

        void async_connect_ssl( const std::string &address,
                                unsigned short     service,
                                common::system_closure_type closure,
                                const std::string &verfil,
                                bool tcp_nodelay )
        {
            vtrc::shared_ptr<client_ssl>
                          new_client( create_client_ssl(verfil, tcp_nodelay ) );

            new_client->async_connect( address, service,
                    vtrc::bind( &this_type::async_connect_success, this,
                                ph::error, closure ));
        }

#endif

        static
        void on_ready_s( vtrc::condition_variable *cond )
        {
            cond->notify_all( );
        }

        static
        void on_init_error_s( unsigned *failed,
                              std::string *res,
                              vtrc::condition_variable *cond,
                              const rpc::errors::container &,
                              const char *message  )
        {
            (*failed) = 1;
            res->assign( message );
            cond->notify_all( );
        }

        static
        void on_disconnect_s( vtrc::condition_variable *cond )
        {
            cond->notify_all( );
        }

        bool on_ready_diconnect( unsigned *failed )
        {
            return ((*failed) != 0) || ready( );
        }

        template <typename FuncType>
        void connect_impl( FuncType conn_func )
        {
            unsigned                 failed = 0;
            std::string              failed_message;
            vtrc::condition_variable cond;
            vtrc::mutex              cond_lock;

            bs2::scoped_connection rc(
                        parent_->on_ready_connect(
                            vtrc::bind( impl::on_ready_s, &cond ) ) );

            bs2::scoped_connection fc( parent_->on_init_error_connect(
                            vtrc::bind( impl::on_init_error_s,
                                        &failed, &failed_message,
                                        &cond, ph::_1, ph::_2 )) );

            vtrc::unique_lock<vtrc::mutex> ul(cond_lock);

            /// call connect
            conn_func( );

            parent_->get_on_connect( )( );

            bool ok = cond.wait_for( ul,
                      vtrc::chrono::seconds( 10 ),
                      vtrc::bind( &impl::on_ready_diconnect, this, &failed ) );

            if( !ok ) {
                vtrc::common::raise(
                    vtrc::common::exception( rpc::errors::ERR_TIMEOUT ) );
                return;
            }

            if( failed != 0 ) {
                vtrc::common::raise(
                    vtrc::common::exception( rpc::errors::ERR_INTERNAL,
                                             failed_message ) );
                return;
            }

        }

        void connect( const std::string &address,
                      unsigned short service, bool tcp_nodelay )
        {
            vtrc::shared_ptr<client_tcp>
                            new_client(create_client_tcp(tcp_nodelay));

            connect_impl(vtrc::bind( &client_tcp::connect, new_client,
                                     address, service));
            if( tcp_nodelay ) {
                new_client->set_no_delay( true );
            }
        }

#ifndef _WIN32
        void open( const std::string &path, int flags, int mode )
        {
            vtrc::shared_ptr<client_posixs>
                   new_client(client_posixs::create( parent_, this ));
            new_client->connect( path, flags, mode );
        }
#endif

#ifdef _WIN32
        static void win_connect( client_win_pipe &new_client,
                                 const std::string &local_name)
        {
            new_client.connect( local_name );
        }

        static void win_connect_w( client_win_pipe &new_client,
                                   const std::wstring &local_name)
        {
            new_client.connect( local_name );
        }
#endif

        void connect( const std::string &local_name )
        {

#ifdef _WIN32
            vtrc::shared_ptr<client_win_pipe>
                             new_client(create_client<client_win_pipe>( ));
            connect_impl(vtrc::bind( &impl::win_connect,
                                      vtrc::ref(*new_client),
                                      vtrc::ref(local_name)));
#else
            vtrc::shared_ptr<client_unix_local>
                            new_client(create_client<client_unix_local>( ));
            connect_impl(vtrc::bind( &client_unix_local::connect, new_client,
                                     local_name));
#endif
        }

        void async_connect_success( const bsys::error_code &err,
                                    common::system_closure_type closure )
        {
            if( !err ) {
                parent_->get_on_connect( )( );
            } else {
                rpc::errors::container errc;
                errc.set_code( err.value( ) );
                errc.set_category( rpc::errors::CATEGORY_SYSTEM );
                errc.set_fatal( true );
                errc.set_additional( err.message( ) );
                parent_->get_on_init_error( )( errc, err.message( ).c_str() );
            }

            closure(err);
        }

#ifdef _WIN32
        void connect( const std::wstring &local_name )
        {
            vtrc::shared_ptr<client_win_pipe>
                         new_client(create_client<client_win_pipe>( ));
            connect_impl(vtrc::bind( &impl::win_connect_w,
                                      vtrc::ref(*new_client),
                                      vtrc::ref(local_name)));
        }

        void async_connect( const std::wstring &local_name,
                            common::system_closure_type closure )
        {
            vtrc::shared_ptr<client_win_pipe>
                         new_client(create_client<client_win_pipe>( ));

            new_client->async_connect( local_name,
                vtrc::bind( &this_type::async_connect_success, this,
                            ph::error, closure ));
        }
#endif
        void async_connect( const std::string &local_name,
                            common::system_closure_type closure )
        {

#ifdef _WIN32
            vtrc::shared_ptr<client_win_pipe>
                         new_client(create_client<client_win_pipe>( ));

            new_client->async_connect( local_name,
                vtrc::bind( &this_type::async_connect_success,
                            this, ph::error, closure ));
#else
            vtrc::shared_ptr<client_unix_local>
                         new_client(create_client<client_unix_local>( ));

            new_client->async_connect( local_name,
            vtrc::bind( &this_type::async_connect_success,
                         this, ph::error, closure ));
#endif
        }

        void async_connect( const std::string &address,
                            unsigned short     service,
                            bool tcp_nodelay,
                            common::system_closure_type &closure )
        {
            vtrc::shared_ptr<client_tcp>
                               new_client(create_client_tcp( tcp_nodelay ));

            new_client->async_connect( address, service,
                    vtrc::bind( &this_type::async_connect_success,
                                this, ph::error, closure ));
        }
    };

    vtrc_client::vtrc_client( VTRC_ASIO::io_service &ios,
                              VTRC_ASIO::io_service &rpc_ios )
        :client::base(ios, rpc_ios)
        ,impl_(new impl)
    {
        impl_->parent_ = this;
    }

    vtrc_client::vtrc_client( VTRC_ASIO::io_service &ios )
        :client::base(ios, ios)
        ,impl_(new impl)
    {
        impl_->parent_ = this;
    }

    vtrc::shared_ptr<vtrc_client> vtrc_client::create(basio::io_service &ios)
    {
        vtrc::shared_ptr<vtrc_client> new_inst( new vtrc_client( ios ) );
        return new_inst;
    }

    vtrc::shared_ptr<vtrc_client> vtrc_client::create(basio::io_service &ios,
                                                  basio::io_service &rpc_ios)
    {
        vtrc::shared_ptr<vtrc_client> new_inst( new vtrc_client( ios, rpc_ios));
        return new_inst;
    }

    vtrc::shared_ptr<vtrc_client> vtrc_client::create( common::pool_pair &pool )
    {
        vtrc::shared_ptr<vtrc_client>
                new_inst( new vtrc_client(
                              pool.get_io_service( ),
                              pool.get_rpc_service( )));
        return new_inst;
    }

    vtrc_client::~vtrc_client( )
    {
        delete impl_;
    }

    void vtrc_client::set_session_key( const std::string &id,
                                       const std::string &key )
    {
        impl_->set_session_key( id, key );
    }

    void vtrc_client::set_session_key( const std::string &key )
    {
        impl_->set_session_key( key );
    }

    std::string vtrc_client::get_session_key( ) const
    {
        return impl_->get_session_key( );
    }

    void vtrc_client::connect(const std::string &local_name)
    {
        impl_->connect( local_name );
    }

#ifndef _WIN32
    void vtrc_client::open( const std::string &path, int flags, int mode )
    {
        impl_->open( path, flags, mode );
    }
#endif

#ifdef _WIN32
    void vtrc_client::connect(const std::wstring &local_name)
    {
        impl_->connect( local_name );
    }
#endif

    void vtrc_client::connect( const std::string &address,
                               unsigned short service, bool tcp_nodelay )
    {
        impl_->connect( address, service, tcp_nodelay );
    }

    void vtrc_client::async_connect( const std::string &local_name,
                                     common::system_closure_type closure )
    {
        impl_->async_connect( local_name, closure );
    }

#if VTRC_OPENSSL_ENABLED
    void vtrc_client::connect_ssl( const std::string &address,
                      unsigned short service,
                      const std::string &verify_file,
                      verify_callback_type cb,
                      bool tcp_nodelay )
    {
        impl_->connect_ssl( address, service, verify_file, cb, tcp_nodelay);
    }

    void vtrc_client::connect_ssl( const std::string &address,
                      unsigned short service,
                      const std::string &verify_file,
                      bool tcp_nodelay )
    {
        impl_->connect_ssl( address, service, verify_file, tcp_nodelay);
    }

    void vtrc_client::async_connect_ssl( const std::string &address,
                            unsigned short     service,
                            common::system_closure_type closure,
                            const std::string &verify_file,
                            verify_callback_type cb,
                            bool tcp_nodelay )
    {
        impl_->async_connect_ssl( address, service, closure,
                                  verify_file, cb, tcp_nodelay );
    }

    void vtrc_client::async_connect_ssl( const std::string &address,
                            unsigned short     service,
                            common::system_closure_type closure,
                            const std::string &verify_file,
                            bool tcp_nodelay )
    {
        impl_->async_connect_ssl( address, service, closure,
                                  verify_file, tcp_nodelay );
    }

#endif

#ifdef _WIN32
    void vtrc_client::async_connect( const std::wstring &local_name,
                                     common::system_closure_type closure )
    {
        impl_->async_connect( local_name, closure );
    }
#endif

    void vtrc_client::async_connect( const std::string &address,
                            unsigned short service,
                            common::system_closure_type closure,
                            bool tcp_nodelay )
    {
        impl_->async_connect( address, service, tcp_nodelay, closure );
    }

    void  vtrc_client::reset_session_key( )
    {
        impl_->reset_session_key( );
    }

    void  vtrc_client::reset_session_info( )
    {
        impl_->reset_session_info( );
    }

    const common::call_context *vtrc_client::get_call_context( )
    {
        return common::call_context::get( );
    }

    void vtrc_client::connect( )
    { }

    void vtrc_client::async_connect( common::system_closure_type )
    { }


}}

