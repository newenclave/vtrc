#include "vtrc-transport-iface.h"

#ifdef _WIN32

#include <boost/asio/windows/stream_handle.hpp>

#include "vtrc-memory.h"

namespace boost {

namespace asio {
    class    io_service;
}

namespace system {
    class error_code;
}}

namespace vtrc { namespace common {

    class transport_win_pipe: public transport_iface {

        struct impl;
        friend struct impl;
        impl *impl_;

        transport_win_pipe( const transport_win_pipe & );
        transport_win_pipe& operator = ( const transport_win_pipe & );

    public:

        typedef boost::asio::windows::stream_handle socket_type;

        transport_win_pipe( vtrc::shared_ptr<socket_type> sock );
        virtual ~transport_win_pipe(  );

        const char *name( ) const                     ;
        void close( )                                 ;

        common::enviroment      &get_enviroment( )    ;

        socket_type             &get_socket( )        ;
        const socket_type       &get_socket( ) const  ;

        void write( const char *data, size_t length ) ;
        void write( const char *data, size_t length, closure_type &success ) ;

        virtual void on_write_error( const boost::system::error_code &err ) = 0;

    private:

        virtual std::string prepare_for_write( const char *data, size_t len );
    };

}}


#endif