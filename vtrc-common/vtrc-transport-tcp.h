
#include "vtrc-transport-iface.h"

#include <boost/asio/ip/tcp.hpp>

namespace boost {

namespace asio {
    class    io_service;
}

namespace system {
    class error_code;
}}

namespace vtrc { namespace common {

    class transport_tcp: public transport_iface {

        struct impl;
        friend struct impl;
        impl *impl_;

        transport_tcp( const transport_tcp & );
        transport_tcp& operator = ( const transport_tcp & );

    public:

        transport_tcp( boost::asio::ip::tcp::socket *sock );
        virtual ~transport_tcp(  );

        const char *name( ) const                     ;
        void close( )                                 ;
        bool active( ) const                          ;

        common::enviroment      &get_enviroment( )    ;

        boost::asio::ip::tcp::socket &get_socket( )   ;
        const boost::asio::ip::tcp::socket &get_socket( ) const;

        void write( const char *data, size_t length ) ;
        void write( const char *data, size_t length, closure_type &success ) ;

        virtual void on_write_error( const boost::system::error_code &err ) = 0;


    private:

        virtual std::string prepare_for_write( const char *data, size_t len );
    };

}}

