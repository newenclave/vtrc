#ifndef VTRC_CLIENT_UNIX_LOCAL_H
#define VTRC_CLIENT_UNIX_LOCAL_H

#ifndef _WIN32

#include "vtrc-common/vtrc-transport-unix-local.h"
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

    class client_unix_local: public common::transport_unix_local {

        struct impl;
        friend struct impl;
        impl  *impl_;

        client_unix_local( boost::asio::io_service &ios, vtrc_client *client );

    public:

        typedef vtrc::shared_ptr<client_unix_local> shared_type;

        static shared_type create (
                            boost::asio::io_service &ios, vtrc_client *client );

        ~client_unix_local( );
        void init( );
        bool active( ) const;

        void connect( const std::string &address );
        void async_connect( const std::string &address,
                            common::closure_type closure );

        void on_write_error( const boost::system::error_code &err );

        const common::call_context *get_call_context( ) const;

    private:
        common::protocol_layer &get_protocol( );
        std::string prepare_for_write( const char *data, size_t len );

    };
}}


#endif

#endif // VTRCCLIENTUNIXLOCAL_H
