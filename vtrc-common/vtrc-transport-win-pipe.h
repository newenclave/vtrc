#include "vtrc-transport-iface.h"

#ifdef _WIN32

#include "vtrc-asio.h"

#include "vtrc-memory.h"
#include "vtrc-common/vtrc-closure.h"

namespace vtrc { namespace common {

    class transport_win_pipe: public transport_iface {

        struct impl;
        friend struct impl;
        impl *impl_;

        transport_win_pipe( const transport_win_pipe & );
        transport_win_pipe& operator = ( const transport_win_pipe & );

    public:

        typedef VTRC_ASIO::windows::stream_handle socket_type;

        transport_win_pipe( vtrc::shared_ptr<socket_type> sock );
        virtual ~transport_win_pipe(  );

        std::string name( ) const                     ;
        void close( )                                 ;

        socket_type             &get_socket( )        ;
        const socket_type       &get_socket( ) const  ;

        void write( const char *data, size_t length ) ;
        void write( const char *data, size_t length,
            const system_closure_type &success, bool on_send_success ) ;

        native_handle_type native_handle( ) const;
        virtual void on_write_error( const VTRC_SYSTEM::error_code &err ) = 0;

    private:

        virtual std::string prepare_for_write( const char *data, size_t len );
    };

}}


#endif
