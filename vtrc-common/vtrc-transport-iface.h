#ifndef VTRC_TRANSPORT_IFACE_H
#define VTRC_TRANSPORT_IFACE_H

namespace boost { namespace asio {
    class io_service;

}}

namespace vtrc { namespace common {

    struct enviroment;

    struct transport_iface {

        virtual ~transport_iface( ) { }
        //
        virtual const char *name( ) const                     = 0;
        virtual void close( )                                 = 0;
        virtual common::enviroment      &get_enviroment( )    = 0;
        virtual boost::asio::io_service &get_io_service( )    = 0;

        virtual void write( const char *data, size_t length ) = 0;

    };

}}

#endif // VTRC_TRANSPORT_IFACE_H
