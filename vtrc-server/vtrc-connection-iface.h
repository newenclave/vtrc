#ifndef VTRC_CONNECTION_IFACE_H
#define VTRC_CONNECTION_IFACE_H

#include <stdlib.h>

//namespace google { namespace protobuf {
//    class RpcChannel;
//}}

#include "vtrc-common/vtrc-transport-iface.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc {

    namespace common { class enviroment; }

namespace server {

    struct endpoint_iface;

    struct connection_iface: public common::transport_iface {

        virtual ~connection_iface( ) { }

        virtual bool ready( ) const         = 0;
        virtual endpoint_iface &endpoint( ) = 0;
    };

}}

#endif // VTRC_CONNECTION_IFACE_H
