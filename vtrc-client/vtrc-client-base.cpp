#include "vtrc-client-base.h"

#include "boost/asio/io_service.hpp"

#include "vtrc-rpc-channel-c.h"
#include "vtrc-protocol-layer-c.h"

namespace vtrc { namespace client {

    namespace basio = VTRC_ASIO;

    struct base::impl {
        basio::io_service              *ios_;
        basio::io_service              *rpc_ios_;
        base                           *parent_;
        common::connection_iface_sptr   connection_;
        impl(basio::io_service &io, basio::io_service &rpcio)
            :ios_(&io)
            ,rpc_ios_(&rpcio)
        { }

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

        bool ready( ) const
        {
            return connection_ ? connection_->get_protocol( ).ready( ) : false;
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

}}
