
#include "vtrc-transport-iface.h"

#include <boost/asio/ip/tcp.hpp>

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

        void write( const char *data, size_t length ) ;


    };

}}

