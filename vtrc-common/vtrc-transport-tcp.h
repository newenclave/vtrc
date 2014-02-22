
#include "vtrc-transport-iface.h"

#include <boost/asio/ip/tcp.hpp>

namespace boost {

namespace asio {
    class io_service;
}

namespace system {
    class error_code;
}}

namespace vtrc { namespace common {

    class transport_tcp: public transport_iface {

        struct transport_tcp_impl;
        friend struct transport_tcp_impl;
        transport_tcp_impl *impl_;

    public:

        virtual ~transport_tcp(  );
        transport_tcp( boost::asio::ip::tcp::socket *sock );

        const char *name( ) const                     ;
        void close( )                                 ;
        common::enviroment      &get_enviroment( )    ;
        boost::asio::io_service &get_io_service( )    ;

        boost::asio::ip::tcp::socket &get_socket( )   ;
        const boost::asio::ip::tcp::socket &get_socket( ) const;

        void write( const char *data, size_t length ) ;
        void write( const char *data, size_t length, closure_type &success ) ;
        virtual void on_write_error( const boost::system::error_code &err ) = 0;

        void send_message( const char *data, size_t length );

    private:
        virtual std::string prepare_for_write( const char *data, size_t len );
    };

}}

