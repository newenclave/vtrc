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

    typedef vtrc::function <
                void (const boost::system::error_code &)
            > closure_type;

    class enviroment;
    class protocol_layer;

    struct connection_iface: public enable_shared_from_this<connection_iface> {

        virtual ~connection_iface( ) { }

        virtual const char *name( ) const                  = 0;
        virtual void close( )                              = 0;
        virtual bool active( ) const                       = 0;
        virtual common::enviroment      &get_enviroment( ) = 0;
        virtual boost::asio::io_service &get_io_service( ) = 0;
        virtual protocol_layer          &get_protocol( )   = 0;

        vtrc::weak_ptr<connection_iface> weak_from_this( )
        {
            return vtrc::weak_ptr<connection_iface>( shared_from_this( ) );
        }

        vtrc::weak_ptr<connection_iface const> weak_from_this( ) const
        {
            return vtrc::weak_ptr<connection_iface const>( shared_from_this( ));
        }
    };

    typedef vtrc::shared_ptr<connection_iface> connection_iface_sptr;
    typedef weak_ptr<connection_iface>   connection_iface_wptr;

}}

#endif // VTRCCONNECTIONIFACE_H
