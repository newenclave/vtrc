#ifndef VTRC_CLIENT_WIN_LOCAL_H
#define VTRC_CLIENT_WIN_LOCAL_H

#ifdef _WIN32

#include "vtrc/common/transport/win/local.h"

#include "vtrc/common/lowlevel-protocol-iface.h"
#include "vtrc/common/closure.h"
#include "vtrc/common/config/vtrc-memory.h"

VTRC_ASIO_FORWARD(
    class io_service;
)

namespace vtrc {

namespace common {
    class protocol_layer;
    class call_context;
}

namespace client {

    class   base;
    class   vtrc_client;
    struct  protocol_signals;

    class client_win_pipe: public common::transport_win_pipe {

        struct impl;
        impl  *impl_;

        client_win_pipe( VTRC_ASIO::io_service &ios,
                         client::base *client,
                         protocol_signals *callbacks );

    public:

        typedef vtrc::shared_ptr<client_win_pipe> shared_type;
        static shared_type create ( client::base *client,
                                    protocol_signals *callbacks );

        ~client_win_pipe( );
        void init( );
        bool active( ) const;
        const std::string &id( ) const;

        void connect( const std::string &address );
        void connect( const std::wstring &address );

        void async_connect( const std::string &address,
                            common::system_closure_type closure );
        void async_connect( const std::wstring &address,
                            common::system_closure_type closure );

        void on_write_error( const VTRC_SYSTEM::error_code &err );
        const common::call_context *get_call_context( ) const;

        common::protocol_layer       &get_protocol( );
        const common::protocol_layer &get_protocol( ) const;
        common::environment          &get_environment( );

    private:
        void on_close( );
        void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                              common::empty_closure_type done );

    };
}}

#endif
#endif
