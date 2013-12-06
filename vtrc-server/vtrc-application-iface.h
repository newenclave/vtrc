#ifndef VTRC_APPLICATION_IFACE_H
#define VTRC_APPLICATION_IFACE_H

namespace boost { namespace asio {
    class io_service;  
}}

namespace vtrc { namespace server {

    struct application_iface {
        virtual ~application_iface( ) { }
        virtual boost::asio::io_service &get_io_service( ) = 0;
    };

}}

#endif
