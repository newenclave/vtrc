#include "boost/asio.hpp"
#include <map>
#include <vector>
#include <string>

#include "google/protobuf/descriptor.h"

#include "vtrc-client.h"
#include "vtrc-client-tcp.h"
#include "vtrc-client-ssl.h"
#include "vtrc-client-unix-local.h"
#include "vtrc-client-win-pipe.h"
#include "vtrc-rpc-channel-c.h"
#include "vtrc-protocol-layer-c.h"

#include "vtrc-bind.h"
#include "vtrc-ref.h"
#include "vtrc-condition-variable.h"
#include "vtrc-chrono.h"

#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "vtrc-errors.pb.h"

#include "vtrc-rpc-channel-c.h"

namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;
    namespace gpb   = google::protobuf;
    namespace bs2   = boost::signals2;
    namespace ph    = vtrc::placeholders;

    namespace {

        typedef vtrc::weak_ptr<gpb::Service>    service_wptr;
        typedef vtrc::shared_ptr<gpb::Service>  service_sptr;

        typedef vtrc::shared_ptr<gpb::RpcChannel>  channel_sptr;

        typedef std::map< std::string, service_wptr> service_weak_map;
        typedef std::map< std::string, service_sptr> service_shared_map;

    }

    struct vtrc_client::impl: public protocol_signals {

        typedef impl this_type;

        basio::io_service              &ios_;
        basio::io_service              &rpc_ios_;
        vtrc_client                    *parent_;
        common::protocol_layer         *protocol_;
        common::connection_iface_sptr   connection_;

        service_weak_map                weak_services_;
        service_shared_map              hold_services_;
        vtrc::shared_mutex              services_lock_;

        std::string                     session_key_;
        std::string                     session_id_;
        bool                            key_set_;
        //vtrc::mutex                     session_info_lock_;

        service_factory_type                 factory_;
        vtrc::shared_mutex              factory_lock_;

        impl( basio::io_service &ios, basio::io_service &rpc_ios )
            :ios_(ios)
            ,rpc_ios_(rpc_ios)
            ,protocol_(NULL)
            ,key_set_(false)
        { }

        /// ============= signals =============== /////
        void on_init_error(const rpc::errors::container &err,
                                   const char *message)
        {
            parent_->on_init_error_( err, message );
        }

        void on_connect( )
        {
            parent_->on_connect_( );
        }

        void on_disconnect( )
        {
            parent_->on_disconnect_( );
        }

        void on_ready( )
        {
            parent_->on_ready_( );
        }
        /// ====================================== /////

        const common::call_context *get_call_context( ) const
        {
            return (connection_ ? connection_->get_call_context( ) : NULL);
        }

        void set_session_key( const std::string &id, const std::string &key )
        {
            //vtrc::lock_guard<vtrc::mutex> lck(session_info_lock_);
            key_set_     = true;
            session_id_.assign( id );
            session_key_.assign( key );
        }

        void set_session_id( const std::string &id )
        {
            //vtrc::lock_guard<vtrc::mutex> lck(session_info_lock_);
            session_id_.assign( id );
        }

        const std::string &get_session_key(  ) const
        {
            //vtrc::lock_guard<vtrc::mutex> lck(session_info_lock_);
            return session_key_;
        }

        const std::string &get_session_id(  ) const
        {
            //vtrc::lock_guard<vtrc::mutex> lck(session_info_lock_);
            return session_id_;
        }

        bool is_key_set( ) const
        {
            //vtrc::lock_guard<vtrc::mutex> lck(session_info_lock_);
            return key_set_;
        }

        void  reset_session_id( )
        {
            //vtrc::lock_guard<vtrc::mutex> lck(session_info_lock_);
            session_id_.clear( );
        }

        void  reset_session_key( )
        {
            //vtrc::lock_guard<vtrc::mutex> lck(session_info_lock_);
            session_key_.clear( );
            key_set_ = false;
        }

        void  reset_session_info( )
        {
            reset_session_id( );
            reset_session_key( );
        }

        template<typename ClientType>
        vtrc::shared_ptr<ClientType> create_client(  )
        {
            vtrc::shared_ptr<ClientType> c(
                        ClientType::create( ios_, parent_, this ));

            connection_ =   c;
            protocol_   =  &c->get_protocol( );
            return c;
        }

        vtrc::shared_ptr<client_tcp> create_client_tcp( bool tcp_nodelay )
        {
            vtrc::shared_ptr<client_tcp> new_client_inst
                    (client_tcp::create( ios_, parent_, this, tcp_nodelay ));

            connection_ =   new_client_inst;
            protocol_   =  &new_client_inst->get_protocol( );

            return new_client_inst;
        }

#if VTRC_OPENSSL_ENABLED

        vtrc::shared_ptr<client_ssl> create_client_ssl(
                        const std::string &verify_file, bool tcp_nodelay )
        {
            vtrc::shared_ptr<client_ssl> new_client_inst
                    (client_ssl::create( ios_, parent_, this,
                                         verify_file, tcp_nodelay ));

            connection_ =   new_client_inst;
            protocol_   =  &new_client_inst->get_protocol( );

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

            parent_->on_connect_( );

            bool ok = cond.wait_for( ul,
                      vtrc::chrono::seconds( 10 ),
                      vtrc::bind( &impl::on_ready_diconnect, this, &failed ) );

            if( !ok ) {
                throw vtrc::common::exception( rpc::errors::ERR_TIMEOUT );
            }

            if( failed != 0 ) {
                throw vtrc::common::exception( rpc::errors::ERR_INTERNAL,
                                               failed_message);
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

        common::connection_iface_sptr connection( )
        {
            return connection_;
        }

        void async_connect_success( const bsys::error_code &err,
                                    common::system_closure_type closure )
        {
            if( !err ) {
                parent_->on_connect_( );
            } else {
                rpc::errors::container errc;
                errc.set_code( err.value( ) );
                errc.set_category( rpc::errors::CATEGORY_SYSTEM );
                errc.set_fatal( true );
                errc.set_additional( err.message( ) );
                parent_->on_init_error_( errc, err.message( ).c_str() );
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

        bool ready( ) const
        {
            return ( protocol_ && protocol_->ready( ) );
        }

        void disconnect( )
        {
            if( connection_ && connection_->active( ) ) {
                try {
                    connection_->close( );
                } catch( ... ) { }
            };
            connection_.reset( );
        }

        rpc_channel_c *create_channel( )
        {
            rpc_channel_c *new_ch( new rpc_channel_c( connection_ ) );
            return new_ch;
        }

        rpc_channel_c *create_channel( unsigned opts )
        {
            rpc_channel_c *new_ch( new rpc_channel_c( connection_, opts ) );
            return new_ch;
        }

        void assign_handler( vtrc::shared_ptr<gpb::Service> serv )
        {
            const std::string serv_name(serv->GetDescriptor( )->full_name( ));
            vtrc::upgradable_lock lk(services_lock_);
            service_shared_map::iterator f( hold_services_.find( serv_name ) );
            if( f != hold_services_.end( ) ) {
                f->second = serv;
                upgrade_to_unique ulk(lk);
                weak_services_[serv_name] = service_wptr(serv);
            } else {
                upgrade_to_unique ulk(lk);
                hold_services_.insert( std::make_pair(serv_name, serv) );
                weak_services_[serv_name] = service_wptr(serv);
            }
        }

        void assign_weak_handler( vtrc::weak_ptr<gpb::Service> serv )
        {
            service_sptr lock(serv.lock( ));
            if( lock ) {
                const std::string s_name(lock->GetDescriptor( )->full_name( ));
                vtrc::unique_shared_lock lk(services_lock_);
                weak_services_[s_name] = serv;
            }
        }

        void assign_service_factory( service_factory_type factory )
        {
            vtrc::unique_shared_lock lck(factory_lock_);
            factory_ = factory;
        }

        service_sptr get_handler( const std::string &name )
        {
            vtrc::upgradable_lock lk(services_lock_);
            service_sptr result;

            service_weak_map::iterator f( weak_services_.find( name ) );
            if( f != weak_services_.end( ) ) {
                result = f->second.lock( );
                if( !result ) {
                    vtrc::upgrade_to_unique ulk(lk);
                    weak_services_.erase( f );
                }
            }

            if( !result && factory_ ) {
                vtrc::shared_lock shl( factory_lock_ );
                result = factory_( name );
            }

            return result;
        }

        void erase_rpc_handler( const std::string &name )
        {
            vtrc::unique_shared_lock lk(services_lock_);

            service_shared_map::iterator sf( hold_services_.find( name ) );
            service_weak_map::iterator   wf( weak_services_.find( name ) );

            if( sf != hold_services_.end( ) ) hold_services_.erase( sf );
            if( wf != weak_services_.end( ) ) weak_services_.erase( wf );
        }

    };

    vtrc_client::vtrc_client( boost::asio::io_service &ios,
                              boost::asio::io_service &rpc_ios )
        :impl_(new impl(ios, rpc_ios))
    {
        impl_->parent_ = this;
    }

    vtrc_client::vtrc_client( boost::asio::io_service &ios )
        :impl_(new impl(ios, ios))
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

    vtrc::weak_ptr<vtrc_client> vtrc_client::weak_from_this( )
    {
        return vtrc::weak_ptr<vtrc_client>( shared_from_this( ) );
    }

    vtrc::weak_ptr<vtrc_client const> vtrc_client::weak_from_this( ) const
    {
        return vtrc::weak_ptr<vtrc_client const>( shared_from_this( ) );
    }

    vtrc_client::~vtrc_client( )
    {
        delete impl_;
    }

    common::connection_iface_sptr vtrc_client::connection( )
    {
        return impl_->connection( );
    }

    boost::asio::io_service &vtrc_client::get_io_service( )
    {
        return impl_->ios_;
    }

    const boost::asio::io_service &vtrc_client::get_io_service( ) const
    {
        return impl_->ios_;
    }

    boost::asio::io_service &vtrc_client::get_rpc_service( )
    {
        return impl_->rpc_ios_;
    }

    const boost::asio::io_service &vtrc_client::get_rpc_service( ) const
    {
        return impl_->rpc_ios_;
    }

    common::rpc_channel *vtrc_client::create_channel( )
    {
        return impl_->create_channel( );
    }

    common::rpc_channel *vtrc_client::create_channel( unsigned flags )
    {
        return impl_->create_channel( flags );
    }

    void vtrc_client::set_session_key( const std::string &id,
                                       const std::string &key )
    {
        impl_->set_session_key( id, key );
    }

    void vtrc_client::set_session_key( const std::string &key )
    {
        set_session_key( impl_->get_session_id( ), key );
    }

    void vtrc_client::set_session_id( const std::string &id )
    {
        impl_->set_session_id( id );
    }

    const std::string &vtrc_client::get_session_key( ) const
    {
        return impl_->get_session_key( );
    }

    const std::string &vtrc_client::get_session_id( ) const
    {
        return impl_->get_session_id( );
    }

    bool vtrc_client::is_key_set(  ) const
    {
        return impl_->is_key_set( );
    }

    const common::call_context *vtrc_client::get_call_context( ) const
    {
        return impl_->get_call_context( );
    }

    void vtrc_client::connect(const std::string &local_name)
    {
        impl_->connect( local_name );
    }

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

    bool vtrc_client::ready( ) const
    {
        return impl_->ready( );
    }

    void vtrc_client::disconnect( )
    {
        impl_->disconnect( );
    }

    void vtrc_client::assign_rpc_handler( vtrc::shared_ptr<gpb::Service> serv )
    {
        impl_->assign_handler( serv );
    }

    void vtrc_client::assign_weak_rpc_handler(vtrc::weak_ptr<gpb::Service> serv)
    {
        impl_->assign_weak_handler( serv );
    }

    void vtrc_client::assign_service_factory( service_factory_type factory )
    {
        impl_->assign_service_factory( factory );
    }

    service_sptr vtrc_client::get_rpc_handler( const std::string &name )
    {
        return impl_->get_handler( name );
    }

    void vtrc_client::erase_rpc_handler(vtrc::shared_ptr<gpb::Service> handler)
    {
        impl_->erase_rpc_handler( handler->GetDescriptor( )->full_name( ) );
    }

    void vtrc_client::erase_rpc_handler(const std::string &name)
    {
        impl_->erase_rpc_handler( name );
    }

    void  vtrc_client::reset_session_id( )
    {
        impl_->reset_session_id( );
    }

    void  vtrc_client::reset_session_key( )
    {
        impl_->reset_session_key( );
    }

    void  vtrc_client::reset_session_info( )
    {
        impl_->reset_session_info( );
    }

}}

