#ifndef VTRC_TRANSPORT_POSIX_STREAM_H
#define VTRC_TRANSPORT_POSIX_STREAM_H

#ifndef _WIN32

#include "vtrc-transport-iface.h"

#include "vtrc-asio.h"
#include "vtrc-memory.h"
#include "vtrc/common/closure.h"

VTRC_ASIO_FORWARD( class io_service;)

namespace vtrc { namespace common {

    class transport_posixs: public transport_iface {

        struct impl;
        friend struct impl;
        impl *impl_;

        transport_posixs( const transport_posixs & );
        transport_posixs& operator = ( const transport_posixs & );

    public:

        typedef VTRC_ASIO::posix::stream_descriptor socket_type;

        transport_posixs( vtrc::shared_ptr<socket_type> sock );
        virtual ~transport_posixs(  );

        std::string  name( ) const                     ;
        void close( )                                 ;

        socket_type             &get_socket( )        ;
        const socket_type       &get_socket( ) const  ;

        void write( const char *data, size_t length ) ;
        void write(const char *data, size_t length,
                   const system_closure_type &success, bool on_send_success) ;

        native_handle_type native_handle( ) const;

        virtual void on_write_error( const VTRC_SYSTEM::error_code &err ) = 0;
    };

}}

#endif //_WIN32

#endif // VTRC_TRANSPORT_POSIX_STREAM_H

