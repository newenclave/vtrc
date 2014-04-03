#ifndef VTRC_CLIENT_WIN_PIPE_H
#define VTRC_CLIENT_WIN_PIPE_H
#ifdef _WIN32

#include "vtrc-common/vtrc-transport-win-pipe.h"
#include "vtrc-memory.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc {

namespace common {
    class protocol_layer;
}

namespace client {

    class vtrc_client;

    class client_win_pipe: public common::transport_win_pipe {

        struct impl;
        impl  *impl_;

        client_win_pipe( boost::asio::io_service &ios, vtrc_client *client );

    public:

        typedef vtrc::shared_ptr<client_win_pipe> shared_type;

        static shared_type create (
                            boost::asio::io_service &ios, vtrc_client *client );

        ~client_win_pipe( );
        void init( );

        void connect( const std::string &address );
        void connect( const std::wstring &address );
        void async_connect( const std::string &address,
                            common::closure_type closure );
        void async_connect( const std::wstring &address,
                            common::closure_type closure );

        void on_write_error( const boost::system::error_code &err );
        const call_context *get_call_context( );

    private:

        common::protocol_layer &get_protocol( );
        std::string prepare_for_write( const char *data, size_t len );

    };
}}

#endif
#endif
