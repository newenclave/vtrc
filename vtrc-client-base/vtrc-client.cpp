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

        typedef vtrc::weak_ptr<gpb::Service> service_wptr;
        typedef vtrc::shared_ptr<gpb::Service> service_sptr;

        typedef std::map< std::string, service_wptr> service_weak_map;
        typedef std::map< std::string, service_sptr> service_shared_map;
    }

    struct vtrc_client::impl {

        typedef impl this_type;

        basio::io_service              &ios_;
        basio::io_service              &rpc_ios_;
        vtrc_client                    *parent_;
        common::connection_iface_sptr   connection_;

        service_weak_map                weak_services_;
        service_shared_map              hold_services_;
        vtrc::shared_mutex              services_lock_;

        impl( basio::io_service &ios, basio::io_service &rpc_ios )
            :ios_(ios)
            ,rpc_ios_(rpc_ios)
        { }

        void connect( const std::string &address,
                      const std::string &service )
        {
            vtrc::shared_ptr<client_tcp>
                               new_client(client_tcp::create( ios_, parent_ ));
            new_client->connect( address, service );
            connection_ = new_client;
        }

        void connect(const std::string &local_name)
        {
#ifndef _WIN32
            vtrc::shared_ptr<client_unix_local>
                         new_client(client_unix_local::create( ios_, parent_ ));
            new_client->connect( local_name );
            connection_ = new_client;
#else
            vtrc::shared_ptr<client_win_pipe>
                         new_client(client_win_pipe::create( ios_, parent_ ));
            new_client->connect( local_name );
            connection_ = new_client;
#endif
        }

        common::connection_iface_sptr connection( )
        {
            return connection_;
        }

        void async_connect_success( const bsys::error_code &err,
                                    common::closure_type closure )
        {
            closure(err);
        }

#ifdef _WIN32
        void connect(const std::wstring &local_name)
        {
            vtrc::shared_ptr<client_win_pipe>
                         new_client(client_win_pipe::create( ios_, parent_ ));
            new_client->connect( local_name );
            connection_ = new_client;
        }

        void async_connect(const std::wstring &local_name,
                           common::closure_type closure)
        {
            vtrc::shared_ptr<client_win_pipe>
                         new_client(client_win_pipe::create( ios_ , parent_));

            new_client->async_connect( local_name,
                        vtrc::bind( &this_type::async_connect_success, this,
                                     basio::placeholders::error,
                                     closure ));
            connection_ = new_client;
        }
#endif
        void async_connect(const std::string &local_name,
                           common::closure_type closure)
        {
#ifndef _WIN32
            vtrc::shared_ptr<client_unix_local>
                         new_client(client_unix_local::create( ios_ , parent_));

            new_client->async_connect( local_name,
                        vtrc::bind( &this_type::async_connect_success, this,
                                     basio::placeholders::error,
                                     closure ));
            connection_ = new_client;
#else
            vtrc::shared_ptr<client_win_pipe>
                         new_client(client_win_pipe::create( ios_ , parent_));

            new_client->async_connect( local_name,
                        vtrc::bind( &this_type::async_connect_success, this,
                                     basio::placeholders::error,
                                     closure ));
            connection_ = new_client;
#endif
        }

        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type &closure )
        {
            vtrc::shared_ptr<client_tcp>
                               new_client(client_tcp::create( ios_ , parent_));

            new_client->async_connect( address, service,
                        vtrc::bind( &this_type::async_connect_success, this,
                                     basio::placeholders::error,
                                     closure ));
            connection_ = new_client;
        }

        vtrc::shared_ptr<gpb::RpcChannel> create_channel( )
        {
            vtrc::shared_ptr<rpc_channel_c>
               new_ch(vtrc::make_shared<rpc_channel_c>( connection_ ));

            return new_ch;
        }

        vtrc::shared_ptr<gpb::RpcChannel> create_channel( bool dw, bool ins )
        {
            vtrc::shared_ptr<rpc_channel_c>
               new_ch(vtrc::make_shared<rpc_channel_c>( connection_, dw, ins ));

            return new_ch;
        }

        void clean_dead_handlers( )
        {
            ;;;
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

        void erase_rpc_handler(const std::string &name)
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

    vtrc::shared_ptr<gpb::RpcChannel> vtrc_client::create_channel( )
    {
        return impl_->create_channel( );
    }

    vtrc::shared_ptr<google::protobuf::RpcChannel>
                    vtrc_client::create_channel(bool dont_wait, bool insertion)
    {
        return impl_->create_channel( dont_wait, insertion );
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

