#ifndef VTRC_CLIENT_BASE_H
#define VTRC_CLIENT_BASE_H

#include "vtrc-memory.h"
#include "vtrc-function.h"
#include "vtrc-asio-forward.h"
#include "vtrc-system-forward.h"

#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace vtrc {

namespace rpc {
    class lowlevel_unit;
}

namespace rpc { namespace errors {
    class container;
}}

namespace client {

    //// IS NOT YET COMPLETE
    class base: public vtrc::enable_shared_from_this<base> {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        base( const base &other );
        base & operator = ( const base &other );

        VTRC_DECLARE_SIGNAL( on_init_error,
                             void( const rpc::errors::container &,
                                   const char *message ) );

        VTRC_DECLARE_SIGNAL( on_connect,    void( ) );

        VTRC_DECLARE_SIGNAL( on_disconnect, void( ) );
        VTRC_DECLARE_SIGNAL( on_ready,      void( ) );

    protected:

        base( VTRC_ASIO::io_service &ios );
        base( VTRC_ASIO::io_service &ios,
              VTRC_ASIO::io_service &rpc_ios );
        base( common::pool_pair &pools );

    public:

        virtual void connect( ) = 0;
        virtual void async_connect( common::system_closure_type ) = 0;

    public:
        vtrc::weak_ptr<base>         weak_from_this( );
        vtrc::weak_ptr<base const>   weak_from_this( ) const;

        VTRC_ASIO::io_service       &get_io_service( );
        const VTRC_ASIO::io_service &get_io_service( ) const;

        VTRC_ASIO::io_service       &get_rpc_service( );
        const VTRC_ASIO::io_service &get_rpc_service( ) const;

        common::rpc_channel         *create_channel( );
        common::rpc_channel         *create_channel( unsigned flags );

        bool ready( ) const;
        void disconnect( );
    };
}}

#endif // VTRC_CLIENT_BASE_H
