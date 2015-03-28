#ifndef VTRC_TRANSPORT_UNIX_LOCAL_H
#define VTRC_TRANSPORT_UNIX_LOCAL_H

#ifndef  _WIN32

#include "vtrc-transport-iface.h"
#include "boost/asio/local/stream_protocol.hpp"
#include "vtrc-common/vtrc-closure.h"

namespace boost {

    namespace asio {
        class    io_service;
    }

    namespace system {
        class error_code;
    }

}

namespace vtrc { namespace common {

    class transport_unix_local: public transport_iface {

        struct impl;
        friend struct impl;
        impl *impl_;

        transport_unix_local( const transport_unix_local & );
        transport_unix_local& operator = ( const transport_unix_local & );

    public:

        typedef boost::asio::local::stream_protocol::socket socket_type;

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

        native_handle_type native_hanlde( ) const;

        virtual void on_write_error( const boost::system::error_code &err ) = 0;

    private:

        virtual std::string prepare_for_write( const char *data, size_t len );
    };

}}

#endif

#endif // VTRC_TRANSPORT_UNIX_LOCAL_H
