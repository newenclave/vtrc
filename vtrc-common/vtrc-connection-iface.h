#ifndef VTRC_CONNECTION_IFACE_H
#define VTRC_CONNECTION_IFACE_H

#include <boost/function.hpp>

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

    struct connection_iface {

        virtual ~connection_iface( ) { }

        virtual const char *name( ) const                            = 0;
        virtual void close( )                                        = 0;
        virtual common::enviroment      &get_enviroment( )           = 0;
        virtual boost::asio::io_service &get_io_service( )           = 0;

        virtual void send_message( const char *data, size_t length ) = 0;

    };

}}

#endif // VTRCCONNECTIONIFACE_H
