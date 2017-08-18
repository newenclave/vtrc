#ifndef VTRC_CLIENT_POSIX_STREAM_H
#define VTRC_CLIENT_POSIX_STREAM_H

#ifndef _WIN32

#include "vtrc/common/transport/posix-stream.h"
#include "vtrc/common/lowlevel-protocol-iface.h"
#include "vtrc-memory.h"

VTRC_ASIO_FORWARD( class io_service; )

namespace vtrc {

namespace common {
    class protocol_layer;
}

namespace client {

    class  base;
    struct protocol_signals;

    class client_posixs: public common::transport_posixs {

        struct impl;
        friend struct impl;
        impl  *impl_;

        client_posixs( VTRC_ASIO::io_service &ios,
                       client::base *client ,
                       protocol_signals *callbacks);

    public:

        typedef vtrc::shared_ptr<client_posixs> shared_type;

        static shared_type create ( client::base *client,
                                    protocol_signals *callbacks );

        ~client_posixs( );
        void init( );
        bool active( ) const;

        const std::string &id( ) const;

        void connect( const std::string &path, int flags, int mode );
        void async_connect( const std::string &address, int flags, int mode,
                            common::system_closure_type closure );

        void on_write_error( const VTRC_SYSTEM::error_code &err );

        const common::call_context *get_call_context( ) const;

        common::protocol_layer       &get_protocol( );
        const common::protocol_layer &get_protocol( ) const;
        common::environment           &get_enviroment( );

    private:
        void on_close( );
        void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                              common::empty_closure_type done );

    };
}}

#endif // _WIN32

#endif // VTRCCLIENTPOSIXSTREAM_H

