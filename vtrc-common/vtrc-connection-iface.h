#ifndef VTRC_CONNECTION_IFACE_H
#define VTRC_CONNECTION_IFACE_H

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace boost {

    namespace system {
        class error_code;
    }

    namespace asio {
        class io_service;
    }
}

namespace vtrc { namespace common {

    typedef boost::function <
                void (const boost::system::error_code &)
            > closure_type;

    class enviroment;
    class protocol_layer;

    struct connection_iface {

        virtual ~connection_iface( ) { }

        virtual const char *name( ) const                  = 0;
        virtual void close( )                              = 0;
        virtual common::enviroment      &get_enviroment( ) = 0;
        virtual boost::asio::io_service &get_io_service( ) = 0;
        virtual protocol_layer          &get_protocol( )   = 0;

    };

    typedef boost::shared_ptr<connection_iface> connection_iface_sptr;
    typedef boost::weak_ptr<connection_iface>   connection_iface_wptr;

}}

#endif // VTRCCONNECTIONIFACE_H
