#include <boost/asio.hpp>
#include <map>
#include <vector>
#include <string>

#include <google/protobuf/descriptor.h>

#include "vtrc-client.h"
#include "vtrc-client-tcp.h"
#include "vtrc-client-unix-local.h"
#include "vtrc-client-win-pipe.h"

#include "vtrc-rpc-channel-c.h"
#include "vtrc-bind.h"

#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-common/vtrc-pool-pair.h"

namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;
    namespace gpb   = google::protobuf;

    namespace {

        typedef vtrc::weak_ptr<gpb::Service>    service_wptr;
        typedef vtrc::shared_ptr<gpb::Service>  service_sptr;

        typedef vtrc::shared_ptr<gpb::RpcChannel>  channel_sptr;

        typedef std::map< std::string, service_wptr> service_weak_map;
        typedef std::map< std::string, service_sptr> service_shared_map;

    }

    struct vtrc_client::impl {

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

        impl( basio::io_service &ios, basio::io_service &rpc_ios )
            :ios_(ios)
            ,rpc_ios_(rpc_ios)
            ,protocol_(NULL)
            ,key_set_(false)
        { }


        void set_session_key( const std::string &id, const std::string &key )
        {
            key_set_     = true;
            session_id_.assign( id );
            session_key_.assign( key );
        }

        const std::string &get_session_key(  ) const
        {
            return session_key_;
        }

        const std::string &get_session_id(  ) const
        {
            return session_key_;
        }

        bool is_key_set( ) const
        {
            return key_set_;
        }

        template<typename ClientType>
        vtrc::shared_ptr<ClientType> create_client(  )
        {
            vtrc::shared_ptr<ClientType> c(ClientType::create( ios_, parent_ ));
            connection_ =   c;
            protocol_   =  &c->get_protocol( );
            return c;
        }

        void connect( const std::string &address, const std::string &service )
        {
            vtrc::shared_ptr<client_tcp> client(create_client<client_tcp>( ));
            client->connect( address, service );
            parent_->on_connect_( );
        }

        void connect(const std::string &local_name)
        {
#ifndef _WIN32
            vtrc::shared_ptr<client_unix_local>
                                   client(create_client<client_unix_local>( ));
            client->connect( local_name );
#else
            vtrc::shared_ptr<client_win_pipe>
                    client(create_client<client_win_pipe>( ));
            client->connect( local_name );
#endif
            parent_->on_connect_( );
        }

        common::connection_iface_sptr connection( )
        {
            return connection_;
        }

        void async_connect_success( const bsys::error_code &err,
                                    common::closure_type closure )
        {
            if( !err )
                parent_->on_connect_( );
            closure(err);
        }

#ifdef _WIN32
        void connect(const std::wstring &local_name)
        {
            vtrc::shared_ptr<client_win_pipe>
                         new_client(create_client<client_win_pipe>( ));
            new_client->connect( local_name );
        }

        void async_connect(const std::wstring &local_name,
                           common::closure_type closure)
        {
            vtrc::shared_ptr<client_win_pipe>
                         new_client(create_client<client_win_pipe>( ));

            new_client->async_connect( local_name,
                vtrc::bind( &this_type::async_connect_success,
                            this,
                            basio::placeholders::error,
                            closure ));
        }
#endif
        void async_connect(const std::string &local_name,
                           common::closure_type closure)
        {
#ifndef _WIN32
            vtrc::shared_ptr<client_unix_local>
                         new_client(create_client<client_unix_local>( ));

            new_client->async_connect( local_name,
            vtrc::bind( &this_type::async_connect_success,
                         this,
                         basio::placeholders::error,
                         closure ));
#else
            vtrc::shared_ptr<client_win_pipe>
                         new_client(create_client<client_win_pipe>( ));

            new_client->async_connect( local_name,
                vtrc::bind( &this_type::async_connect_success,
                            this,
                            basio::placeholders::error,
                            closure ));
#endif
        }

        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type &closure )
        {
            vtrc::shared_ptr<client_tcp>
                               new_client(create_client<client_tcp>( ));

            new_client->async_connect( address, service,
                    vtrc::bind( &this_type::async_connect_success,
                                this,
                                basio::placeholders::error,
                                closure ));
        }

        bool ready( ) const
        {
            return (protocol_ && protocol_->ready( ));
        }

        void disconnect( )
        {
            try {
                connection_->close( );
            } catch( ... ) { };
        }

        channel_sptr create_channel( )
        {
            vtrc::shared_ptr<rpc_channel_c>
               new_ch(vtrc::make_shared<rpc_channel_c>( connection_ ));

            return new_ch;
        }

        channel_sptr create_channel( common::rpc_channel::options opts )
        {
            vtrc::shared_ptr<rpc_channel_c>
               new_ch(vtrc::make_shared<rpc_channel_c>( connection_, opts ));

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

    vtrc::shared_ptr<vtrc_client> vtrc_client::create(common::pool_pair &pools)
    {
        vtrc::shared_ptr<vtrc_client>
                new_inst( new vtrc_client(
                              pools.get_io_service( ),
                              pools.get_rpc_service( )));
        return new_inst;
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

    channel_sptr vtrc_client::create_channel( )
    {
        return impl_->create_channel( );
    }

    channel_sptr vtrc_client::create_channel(common::rpc_channel::options opts)
    {
        return impl_->create_channel( opts );
    }

    void vtrc_client::set_session_key(const std::string &id,
                                      const std::string &key )
    {
        impl_->set_session_key( id, key );
    }

    void vtrc_client::set_session_key( const std::string &key )
    {
        static const std::string empty_id;
        set_session_key( empty_id, key );
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
                               const std::string &service )
    {
        impl_->connect( address, service );
    }

    void vtrc_client::async_connect(const std::string &local_name,
                                    common::closure_type closure)
    {
        impl_->async_connect( local_name, closure );
    }

#ifdef _WIN32

    void vtrc_client::async_connect(const std::wstring &local_name,
                                    common::closure_type closure)
    {
        impl_->async_connect( local_name, closure );
    }

#endif

    void vtrc_client::async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type closure )
    {
        impl_->async_connect( address, service, closure );
    }

    bool vtrc_client::ready( ) const
    {
        return impl_->ready( );
    }

    void vtrc_client::disconnect( )
    {
        impl_->disconnect( );
    }

    void vtrc_client::assign_rpc_handler(vtrc::shared_ptr<gpb::Service> serv)
    {
        impl_->assign_handler( serv );
    }

    void vtrc_client::assign_weak_rpc_handler(vtrc::weak_ptr<gpb::Service> serv)
    {
        impl_->assign_weak_handler( serv );
    }

    service_sptr vtrc_client::get_rpc_handler(const std::string &name)
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

}}

