#ifndef VTRC_CLIENT_UNIX_LOCAL_H
#define VTRC_CLIENT_UNIX_LOCAL_H

#ifndef _WIN32

#include "vtrc-common/vtrc-transport-unix-local.h"
#include "vtrc-common/vtrc-lowlevel-protocol-iface.h"
#include "vtrc-memory.h"


namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc {

namespace common {
    class protocol_layer;
}

namespace client {

    class  vtrc_client;
    struct protocol_signals;

    class client_unix_local: public common::transport_unix_local {

        struct impl;
        friend struct impl;
        impl  *impl_;

        client_unix_local(boost::asio::io_service &ios,
                          vtrc_client *client ,
                          protocol_signals *callbacks);

    public:

        typedef vtrc::shared_ptr<client_unix_local> shared_type;
        typedef common::lowlevel_protocol_factory_type lowlevel_factory_type;

        static shared_type create ( boost::asio::io_service &ios,
                                    vtrc_client *client,
                                    protocol_signals *callbacks,
                                    lowlevel_factory_type factory );

        ~client_unix_local( );
        void init( );
        bool active( ) const;

        const std::string &id( ) const;

        void connect( const std::string &address );
        void async_connect( const std::string &address,
                            common::system_closure_type closure );

        void on_write_error( const boost::system::error_code &err );

        const common::call_context *get_call_context( ) const;

        common::protocol_layer     &get_protocol( );
        common::enviroment         &get_enviroment( );

        void assign_protocol_factory( lowlevel_factory_type factory );

    private:
        void on_close( );
        std::string prepare_for_write( const char *data, size_t len );
        void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                              common::empty_closure_type done );

    };
}}


#endif

#endif // VTRCCLIENTUNIXLOCAL_H
