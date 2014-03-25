#include <boost/asio.hpp>
#include <map>
#include <vector>
#include <string>

#include <google/protobuf/descriptor.h>

#include "vtrc-client.h"
#include "vtrc-client-tcp.h"

#include "vtrc-rpc-channel-c.h"
#include "vtrc-bind.h"

#include "vtrc-common/vtrc-mutex-typedefs.h"


namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;
    namespace gpb = google::protobuf;

    namespace {

        typedef vtrc::weak_ptr<gpb::Service> service_wptr;
        typedef vtrc::shared_ptr<gpb::Service> service_sptr;

        typedef std::map< std::string, service_wptr> service_weak_map;
        typedef std::map< std::string, service_sptr> service_shared_map;
    }

    struct vtrc_client::impl {

        typedef impl this_type;

        basio::io_service              &ios_;
        vtrc_client                    *parent_;
        common::connection_iface_sptr   connection_;
        vtrc::shared_ptr<rpc_channel_c> channel_;

        service_weak_map                weak_services_;
        service_shared_map              hold_services_;
        vtrc::shared_mutex              services_lock_;

        impl( basio::io_service &ios )
            :ios_(ios)
        {}

        void connect( const std::string &address,
                      const std::string &service )
        {
            vtrc::shared_ptr<client_tcp>
                               new_client(client_tcp::create( ios_, parent_ ));
            new_client->connect( address, service );
            connection_ = new_client;
            channel_ = vtrc::make_shared<rpc_channel_c>( connection_ );
        }

        common::connection_iface_sptr connection( )
        {
            return connection_;
        }

        void async_connect_success( const bsys::error_code &err,
                                    common::closure_type closure )
        {
            if( !err )
                channel_ = vtrc::make_shared<rpc_channel_c>( connection_ );
            closure(err);
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

        vtrc::shared_ptr<gpb::RpcChannel> get_channel( )
        {
            return channel_;
        }


        void clean_dead_handlers( )
        {
            ;;;
        }

        void advise_handler( vtrc::shared_ptr<gpb::Service> serv )
        {
            const std::string serv_name(serv->GetDescriptor( )->full_name( ));
            vtrc::upgradable_lock lk(services_lock_);
            service_shared_map::iterator f( hold_services_.find( serv_name ) );
            if( f != hold_services_.end( ) ) {
                f->second = serv;
            } else {
                upgrade_to_unique ulk(lk);
                hold_services_.insert( std::make_pair(serv_name, serv) );
                weak_services_[serv_name] = service_wptr(serv);
            }
        }

        void advise_weak_handler( vtrc::weak_ptr<gpb::Service> serv )
        {
            service_sptr lock(serv.lock( ));
            if( lock ) {
                const std::string s_name(lock->GetDescriptor( )->full_name( ));
                vtrc::upgradable_lock lk(services_lock_);
                service_weak_map::iterator f( weak_services_.find( s_name ) );
                if( f != weak_services_.end( ) ) {
                    f->second = serv;
                } else {
                    vtrc::upgrade_to_unique ulk(lk);
                    weak_services_.insert( std::make_pair( s_name, serv) );
                }
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

    };

    vtrc_client::vtrc_client( boost::asio::io_service &ios )
        :impl_(new impl(ios))
    {
        impl_->parent_ = this;
    }

    vtrc::shared_ptr<vtrc_client> vtrc_client::create(basio::io_service &ios)
    {
        vtrc::shared_ptr<vtrc_client> new_inst( new vtrc_client( ios ) );

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

    vtrc::shared_ptr<gpb::RpcChannel> vtrc_client::get_channel( )
    {
        return impl_->get_channel( );
    }

    void vtrc_client::connect( const std::string &address,
                               const std::string &service )
    {
        impl_->connect( address, service );
    }

    void vtrc_client::async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type closure )
    {
        impl_->async_connect( address, service, closure );
    }

    void vtrc_client::advise_handler(vtrc::shared_ptr<gpb::Service> serv)
    {
        impl_->advise_handler( serv );
    }

    void vtrc_client::advise_weak_handler(vtrc::weak_ptr<gpb::Service> serv)
    {
        impl_->advise_weak_handler( serv );
    }

    service_sptr vtrc_client::get_handler(const std::string &name)
    {
        return impl_->get_handler( name );
    }

}}

