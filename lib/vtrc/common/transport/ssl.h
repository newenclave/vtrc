#ifndef VTRC_TRANSPORT_SSL_H
#define VTRC_TRANSPORT_SSL_H

#include "vtrc-general-config.h"

#if VTRC_OPENSSL_ENABLED

#include "vtrc-asio-ssl.h"
#include "vtrc/common/transport/iface.h"

#include "vtrc-asio.h"
#include "vtrc-system-forward.h"

#include "vtrc-memory.h"

#include "vtrc/common/closure.h"

//VTRC_SYSTEM_FORWARD( class error_code; )

namespace vtrc { namespace common {

    class transport_ssl: public transport_iface {

        struct impl;
        friend struct impl;
        impl *impl_;

        transport_ssl( const transport_ssl & );
        transport_ssl& operator = ( const transport_ssl & );

    public:

        typedef VTRC_ASIO::ip::tcp::socket lowlevel_socket_type;
        typedef VTRC_ASIO::ssl::stream<lowlevel_socket_type> socket_type;

        transport_ssl( vtrc::shared_ptr<socket_type> sock );
        virtual ~transport_ssl(  );

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

#endif
