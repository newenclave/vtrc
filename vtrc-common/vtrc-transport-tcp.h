#ifndef VTRC_TRANSPORT_TCP_H
#define VTRC_TRANSPORT_TCP_H

#include "vtrc-transport-iface.h"

#include "vtrc-asio.h"
#include "vtrc-memory.h"
#include "vtrc-common/vtrc-closure.h"

VTRC_ASIO_FORWARD( class io_service;)

namespace vtrc { namespace common {

    class transport_tcp: public transport_iface {

        struct impl;
        friend struct impl;
        impl *impl_;

        transport_tcp( const transport_tcp & );
        transport_tcp& operator = ( const transport_tcp & );

    public:

        typedef VTRC_ASIO::ip::tcp::socket socket_type;

        transport_tcp( vtrc::shared_ptr<socket_type> sock );
        virtual ~transport_tcp(  );

        std::string  name( ) const                     ;
        void close( )                                 ;

        socket_type             &get_socket( )        ;
        const socket_type       &get_socket( ) const  ;

        void write( const char *data, size_t length ) ;
        void write(const char *data, size_t length,
                   const system_closure_type &success, bool on_send_success) ;

        native_handle_type native_handle( ) const;

        virtual void on_write_error( const VTRC_SYSTEM::error_code &err ) = 0;

        void set_no_delay( bool value );

    };

}}

#endif
