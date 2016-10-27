#include "vtrc-client-base.h"

#include "boost/asio/io_service.hpp"
#include "google/protobuf/descriptor.h"

#include "vtrc-rpc-channel-c.h"
#include "vtrc-protocol-layer-c.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"

#include "vtrc-bind.h"


namespace vtrc { namespace client {

    namespace basio = VTRC_ASIO;
    namespace gpb   = google::protobuf;

    namespace {

        typedef vtrc::weak_ptr<gpb::Service>        service_wptr;
        typedef vtrc::shared_ptr<gpb::Service>      service_sptr;
        typedef vtrc::shared_ptr<gpb::RpcChannel>   channel_sptr;

        typedef base::service_wrapper_type service_wrapper_type;
        typedef base::service_wrapper_wptr service_wrapper_wptr;
        typedef base::service_wrapper_sptr service_wrapper_sptr;

        typedef std::map<std::string, service_wptr> native_service_weak_map;
        typedef std::map<std::string, service_wrapper_wptr> service_weak_map;
        typedef std::map<std::string, service_wrapper_sptr> service_shared_map;

        typedef common::protocol_iface::call_type       proto_call_type;
        typedef common::protocol_iface::executor_type   executor_type;

        typedef common::connection_iface_sptr connection_iface_sptr;
    }

    struct base::impl {

        basio::io_service              *ios_;
        basio::io_service              *rpc_ios_;
        base                           *parent_;
        common::connection_iface_sptr   connection_;

        native_service_weak_map         weak_native_services_;
        service_weak_map                weak_services_;
        service_shared_map              hold_services_;
        vtrc::shared_mutex              services_lock_;

        base::service_factory_type      factory_;
        base::lowlevel_factory_type     ll_proto_factory_;

        vtrc::shared_mutex              factory_lock_;

        executor_type                   exec_;

        impl(basio::io_service &io, basio::io_service &rpcio)
            :ios_(&io)
            ,rpc_ios_(&rpcio)
        { }

        void reset_connection( common::connection_iface_sptr new_conn )
        {
            connection_ = new_conn;
        }

        rpc_channel_c *create_channel( )
        {
            rpc_channel_c *new_ch = new rpc_channel_c( connection_ );
            return new_ch;
        }

        rpc_channel_c *create_channel( unsigned opts )
        {
            rpc_channel_c *new_ch = new rpc_channel_c( connection_, opts );
            return new_ch;
        }

        bool ready( ) const
        {
            return connection_ ? connection_->get_protocol( ).ready( )
                               : false;
        }

        void disconnect( )
        {
            if( connection_ && connection_->active( ) ) {
                try {
                    connection_->close( );
                } catch( ... ) { }
            };
            //connection_.reset( );
        }

        void set_default_exec(  )
        {
            namespace ph = vtrc::placeholders;
            exec_ = vtrc::bind( &impl::default_exec, this, ph::_1 );
        }

        void default_exec( proto_call_type call )
        {
            rpc_ios_->post( call );
        }

        void assign_call_executor( base::executor_type exec )
        {
            if( exec ) {
                exec_ = exec;
            } else {
                set_default_exec( );
            }
        }

        void assign_protocol_factory( base::lowlevel_factory_type factory )
        {
            ll_proto_factory_ = factory;
        }

        void assign_weak_handler( vtrc::weak_ptr<gpb::Service> serv )
        {
            service_sptr lock(serv.lock( ));
            if( lock ) {
                const std::string s_name(lock->GetDescriptor( )->full_name( ));
                vtrc::unique_shared_lock lk(services_lock_);
                weak_native_services_[s_name] = serv;
            }
        }

        void assign_handler( vtrc::shared_ptr<gpb::Service> serv )
        {
            assign_handler_wrap( parent_->wrap_service( serv ) );
        }

        void assign_handler_wrap( service_wrapper_sptr serv )
        {
            const std::string serv_name( serv->service( )
                                             ->GetDescriptor( )
                                             ->full_name( ));
            vtrc::upgradable_lock lk(services_lock_);
            service_shared_map::iterator f( hold_services_.find( serv_name ) );
            if( f != hold_services_.end( ) ) {
                f->second = serv;
                upgrade_to_unique ulk(lk);
                weak_services_[serv_name] = service_wrapper_wptr(serv);
            } else {
                upgrade_to_unique ulk(lk);
                hold_services_.insert( std::make_pair(serv_name, serv) );
                weak_services_[serv_name] = service_wrapper_wptr(serv);
            }
        }

        void assign_weak_handler_wrap( service_wrapper_wptr serv )
        {
            service_wrapper_sptr lock(serv.lock( ));
            if( lock ) {
                const std::string s_name(lock->service( )
                                             ->GetDescriptor( )
                                             ->full_name( ));
                vtrc::unique_shared_lock lk(services_lock_);
                weak_services_[s_name] = serv;
            }
        }

        void assign_service_factory( base::service_factory_type factory )
        {
            vtrc::unique_shared_lock lck(factory_lock_);
            factory_ = factory;
        }

        service_wrapper_sptr get_handler( const std::string &name )
        {
            service_wrapper_sptr result;

            {
                vtrc::upgradable_lock lk(services_lock_);
                service_weak_map::iterator f( weak_services_.find( name ) );
                if( f != weak_services_.end( ) ) {
                    result = f->second.lock( );
                    if( !result ) {
                        vtrc::upgrade_to_unique ulk(lk);
                        weak_services_.erase( f );
                    }
                }
            }

            // we don't have service here
            // have to find it in the "native map"
            if( !result ) {
                vtrc::upgradable_lock lk(services_lock_);

                native_service_weak_map::iterator f =
                                weak_native_services_.find( name );

                if( f != weak_native_services_.end( ) ) {
                    service_sptr res = f->second.lock( );
                    if( !res ) {
                        vtrc::upgrade_to_unique ulk(lk);
                        weak_native_services_.erase( f );
                    } else {
                        result = parent_->wrap_service( res );
                    }
                }
            }

            // Ok. last chance to get the service is to create it
            if( !result ) {
                vtrc::shared_lock shl( factory_lock_ );
                if( factory_ ) {
                    result = factory_( name );
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
            weak_native_services_.erase( name );
        }

        void erase_all_rpc_handlers( )
        {
            vtrc::unique_shared_lock lk(services_lock_);
            hold_services_.clear( );
            weak_services_.clear( );
        }
    };

    base::base( VTRC_ASIO::io_service &ios )
        :impl_(new impl(ios, ios))
    {
        impl_->parent_ = this;
    }

    base::base( VTRC_ASIO::io_service &ios,
                VTRC_ASIO::io_service &rpc_ios )
        :impl_(new impl(ios, rpc_ios))
    {
        impl_->parent_ = this;
    }

    base::base( common::pool_pair &pools )
        :impl_(new impl(pools.get_io_service(), pools.get_rpc_service( )))
    {
        impl_->parent_ = this;
    }

    void base::reset_connection( common::connection_iface_sptr new_conn )
    {
        impl_->reset_connection( new_conn );
    }

    common::protocol_iface *base::init_protocol( connection_iface_sptr c )
    {
        /// base!
//        vtrc::unique_ptr<protocol_layer_c> proto
//                        ( new protocol_layer_c( c.get( ), this, impl_ ) );
//        on_connect_( );
//        proto->init( );
//        impl_->connection_ = c;
//        return proto.release( );
        throw std::runtime_error( "not ready!" );
        return nullptr;
    }

    common::connection_iface_sptr base::connection( )
    {
        return impl_->connection_;
    }

    vtrc::weak_ptr<base> base::weak_from_this( )
    {
        return vtrc::weak_ptr<base>(shared_from_this());
    }

    vtrc::weak_ptr<base const> base::weak_from_this( ) const
    {
        return vtrc::weak_ptr<base const>(shared_from_this());
    }

    VTRC_ASIO::io_service &base::get_io_service( )
    {
        return *impl_->ios_;
    }

    const VTRC_ASIO::io_service &base::get_io_service( ) const
    {
        return *impl_->ios_;
    }

    VTRC_ASIO::io_service &base::get_rpc_service( )
    {
        return *impl_->rpc_ios_;
    }

    const VTRC_ASIO::io_service &base::get_rpc_service( ) const
    {
        return *impl_->rpc_ios_;
    }

    common::rpc_channel *base::create_channel( )
    {
        return impl_->create_channel( );
    }

    common::rpc_channel *base::create_channel( unsigned flags )
    {
        return impl_->create_channel(flags);
    }

    bool base::ready( ) const
    {
        return impl_->ready( );
    }

    void base::disconnect( )
    {
        impl_->disconnect( );
    }

    void base::assign_rpc_handler( vtrc::shared_ptr<gpb::Service> serv )
    {
        impl_->assign_handler( serv );
    }

    void base::assign_weak_rpc_handler(vtrc::weak_ptr<gpb::Service> serv)
    {
        impl_->assign_weak_handler( serv );
    }

    void base::assign_rpc_handler( service_wrapper_sptr handler )
    {
        impl_->assign_handler_wrap( handler );
    }

    void base::assign_weak_rpc_handler( service_wrapper_wptr handler)
    {
        impl_->assign_weak_handler_wrap( handler );
    }

    void base::assign_service_factory( service_factory_type factory )
    {
        impl_->assign_service_factory( factory );
    }

    service_wrapper_sptr base::get_rpc_handler( const std::string &name )
    {
        return impl_->get_handler( name );
    }

    void base::erase_rpc_handler( service_sptr handler)
    {
        impl_->erase_rpc_handler( handler->GetDescriptor( )->full_name( ) );
    }

    void base::erase_rpc_handler( const std::string &name )
    {
        impl_->erase_rpc_handler( name );
    }

    void base::erase_all_rpc_handlers( )
    {
        impl_->erase_all_rpc_handlers( );
    }

    void base::assign_lowlevel_protocol_factory( lowlevel_factory_type factory )
    {
        impl_->assign_protocol_factory( factory );
    }

    base::lowlevel_factory_type base::lowlevel_protocol_factory( )
    {
        return impl_->ll_proto_factory_;
    }


    void base::assign_call_executor( executor_type exec )
    {
        impl_->assign_call_executor( exec );
    }

    base::executor_type base::call_executor( )
    {
        return impl_->exec_;
    }

    void base::execute( common::protocol_iface::call_type call )
    {
        impl_->exec_( call );
    }

}}
