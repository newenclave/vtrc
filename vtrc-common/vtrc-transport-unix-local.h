#ifndef VTRC_TRANSPORT_UNIX_LOCAL_H
#define VTRC_TRANSPORT_UNIX_LOCAL_H

#ifndef  _WIN32

#include "vtrc-transport-iface.h"
#include "vtrc-common/vtrc-closure.h"

#include "vtrc-asio-forward.h"
#include "vtrc-system-forward.h"

#include "vtrc-asio.h"

VTRC_ASIO_FORWARD( class io_service; )
VTRC_SYSTEM_FORWARD( class error_code; )

namespace vtrc { namespace common {

    class transport_unix_local: public transport_iface {

        struct impl;
        friend struct impl;
        impl *impl_;

        transport_unix_local( const transport_unix_local & );
        transport_unix_local& operator = ( const transport_unix_local & );

    public:

        typedef VTRC_ASIO::local::stream_protocol::socket socket_type;

        transport_unix_local( vtrc::shared_ptr<socket_type> sock );

        virtual ~transport_unix_local(  );

        std::string name( ) const                     ;
        void close( )                                 ;

        socket_type             &get_socket( )        ;
        const socket_type       &get_socket( ) const  ;

        void write( const char *data, size_t length ) ;
        void write( const char *data, size_t length,
                    const system_closure_type &success,
                    bool on_send_success) ;

        native_handle_type native_handle( ) const;

        virtual void on_write_error( const VTRC_SYSTEM::error_code &err ) = 0;

    };

}}

#endif

#endif // VTRC_TRANSPORT_UNIX_LOCAL_H
