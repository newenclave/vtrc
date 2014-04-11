#ifndef VTRC_CONNECTION_IFACE_H
#define VTRC_CONNECTION_IFACE_H

#include "vtrc-memory.h"
#include "vtrc-function.h"

namespace boost {

    namespace system {
        class error_code;
    }

    namespace asio {
        class io_service;
    }
}

namespace vtrc { namespace common {

    typedef vtrc::function
                       <void (const boost::system::error_code &)> closure_type;

    class enviroment;
    class protocol_layer;
    class call_context;
    class rpc_channel;

    struct connection_iface: public enable_shared_from_this<connection_iface> {

        virtual ~connection_iface( ) { }

        virtual const char *name( ) const                  = 0;

        virtual const std::string &id( ) const             = 0;

        virtual void close( )                              = 0;
        virtual bool active( ) const                       = 0;
        virtual common::enviroment      &get_enviroment( ) = 0;

        virtual const call_context      *get_call_context( ) const = 0;

        virtual bool impersonate( )         { return false; }
        virtual void revert( )              { }

        vtrc::weak_ptr<connection_iface> weak_from_this( )
        {
            return vtrc::weak_ptr<connection_iface>( shared_from_this( ) );
        }

        vtrc::weak_ptr<connection_iface const> weak_from_this( ) const
        {
            return vtrc::weak_ptr<connection_iface const>( shared_from_this( ));
        }

    protected:

        friend class rpc_channel;
        friend class call_keeper;

        virtual protocol_layer &get_protocol( )   = 0;

    };

    typedef vtrc::shared_ptr<connection_iface> connection_iface_sptr;
    typedef vtrc::weak_ptr<connection_iface>   connection_iface_wptr;

}}

#endif // VTRCCONNECTIONIFACE_H
