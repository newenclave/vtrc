#ifndef VTRC_CONNECTION_IFACE_H
#define VTRC_CONNECTION_IFACE_H

#include <stdlib.h>

//namespace google { namespace protobuf {
//    class RpcChannel;
//}}

namespace boost { namespace asio {
    class io_service;

}}

namespace vtrc {

    namespace common { class enviroment; }

namespace server {

    struct endpoint_iface;

    struct connection_iface {
        virtual ~connection_iface( ) { }
        //

        virtual bool ready( ) const                         = 0;
        virtual endpoint_iface &endpoint( )                 = 0;
        virtual const char *name( ) const                   = 0;
        virtual void close( )                               = 0;
        virtual common::enviroment &get_enviroment( )       = 0;
        virtual boost::asio::io_service &get_io_service( )  = 0;

        virtual void write( const char *data, size_t length ) = 0;

    };

}}

#endif // VTRC_CONNECTION_IFACE_H
