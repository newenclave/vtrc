
#include "vtrc-transport-iface.h"

#include "boost/asio/ip/tcp.hpp"

#include "vtrc-memory.h"
#include "vtrc-closure.h"

namespace boost {

namespace asio {
    class io_service;
}

namespace system {
    class error_code;
}}

namespace vtrc { namespace common {

    class protocol_layer;

    class transport_tcp: public transport_iface {

        struct impl;
        friend struct impl;
        impl *impl_;

        transport_tcp( const transport_tcp & );
        transport_tcp& operator = ( const transport_tcp & );

    public:

        typedef boost::asio::ip::tcp::socket socket_type;

        transport_tcp( vtrc::shared_ptr<socket_type> sock );
        virtual ~transport_tcp(  );

        const char *name( ) const                     ;
        void close( )                                 ;

        socket_type             &get_socket( )        ;
        const socket_type       &get_socket( ) const  ;

        void write( const char *data, size_t length ) ;
        void write(const char *data, size_t length,
                   const system_closure_type &success, bool on_send_success) ;

        virtual void on_write_error( const boost::system::error_code &err ) = 0;

    private:
        virtual std::string prepare_for_write( const char *data, size_t len );
    };

}}

