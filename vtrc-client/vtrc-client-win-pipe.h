#ifndef VTRC_CLIENT_WIN_PIPE_H
#define VTRC_CLIENT_WIN_PIPE_H

#ifdef _WIN32

#include "vtrc-common/vtrc-transport-win-pipe.h"
#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-lowlevel-protocol-iface.h"

#include "vtrc-memory.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc {

namespace common {
    class protocol_layer;
    class call_context;
}

namespace client {

    class   vtrc_client;
    struct  protocol_signals;

    class client_win_pipe: public common::transport_win_pipe {

        struct impl;
        impl  *impl_;

        client_win_pipe( boost::asio::io_service &ios, 
                         vtrc_client *client,
                         protocol_signals *callbacks );

    public:

        typedef vtrc::shared_ptr<client_win_pipe> shared_type;
        typedef common::lowlevel_protocol_factory_type lowlevel_factory_type;

        static shared_type create ( boost::asio::io_service &ios, 
                                    vtrc_client *client, 
                                    protocol_signals *callbacks,
                                    lowlevel_factory_type factory );

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

        void on_write_error( const boost::system::error_code &err );
        const common::call_context *get_call_context( ) const;

        common::protocol_layer &get_protocol( );
        common::enviroment     &get_enviroment( );
        void assign_protocol_factory( lowlevel_factory_type factory );

    private:
        void on_close( );
        std::string prepare_for_write( const char *data, size_t len );
        void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                              common::empty_closure_type done );

    };
}}

#endif
#endif
