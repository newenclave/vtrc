#ifndef VTRC_CLIENT_H
#define VTRC_CLIENT_H

#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-common/vtrc-closure.h"

#include "vtrc-memory.h"

namespace boost {

    namespace system {
        class error_code;
    }

    namespace asio {
        class io_service;
    }
}

namespace google { namespace protobuf {
    class RpcChannel;
    class Service;
}}

namespace vtrc {

    namespace rpc {
        class lowlevel_unit;
    }

    namespace rpc { namespace errors {
        class container;
    }}

    namespace common {
        class pool_pair;
        class rpc_channel;
        class call_context;
    }

namespace client {

    class vtrc_client: public vtrc::enable_shared_from_this<vtrc_client> {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        vtrc_client( const vtrc_client &other );
        vtrc_client & operator = ( const vtrc_client &other );

        VTRC_DECLARE_SIGNAL( on_init_error,
                             void( const rpc::errors::container &,
                                   const char *message ) );

        VTRC_DECLARE_SIGNAL( on_connect,    void( ) );

        VTRC_DECLARE_SIGNAL( on_disconnect, void( ) );
        VTRC_DECLARE_SIGNAL( on_ready,      void( ) );

    protected:

        vtrc_client( boost::asio::io_service &ios );

        vtrc_client( boost::asio::io_service &ios,
                     boost::asio::io_service &rpc_ios );

        vtrc_client( common::pool_pair &pools );

    public:

        ~vtrc_client( );

        common::connection_iface_sptr connection( );

        static
        vtrc::shared_ptr<vtrc_client> create( boost::asio::io_service &ios );

        static
        vtrc::shared_ptr<vtrc_client> create(boost::asio::io_service &ios,
                                             boost::asio::io_service &rpc_ios );
        static
        vtrc::shared_ptr<vtrc_client> create( common::pool_pair &pools );

    public:

        vtrc::weak_ptr<vtrc_client>       weak_from_this( );
        vtrc::weak_ptr<vtrc_client const> weak_from_this( ) const;

        boost::asio::io_service       &get_io_service( );
        const boost::asio::io_service &get_io_service( ) const;

        boost::asio::io_service       &get_rpc_service( );
        const boost::asio::io_service &get_rpc_service( ) const;

        common::rpc_channel *create_channel( );
        common::rpc_channel *create_channel( unsigned flags );

        void set_session_key( const std::string &id, const std::string &key );
        void set_session_key( const std::string &key );
        void set_session_id ( const std::string &id );

        const std::string &get_session_key(  ) const;
        const std::string &get_session_id (  ) const;
        bool  is_key_set( ) const;

        void  reset_session_id( );
        void  reset_session_key( );
        void  reset_session_info( ); // key and id

    public:

        const common::call_context *get_call_context( ) const;

    public:

        void connect( const std::string &local_name );

        void connect( const std::string &address,
                      unsigned short service,
                      bool tcp_nodelay = false );

        void async_connect( const std::string &local_name,
                            common::system_closure_type closure);

        void async_connect( const std::string &address,
                            unsigned short     service,
                            common::system_closure_type closure,
                            bool tcp_nodelay = false );
#ifdef _WIN32
        void connect( const std::wstring &local_name );
        void async_connect( const std::wstring &local_name,
                            common::system_closure_type closure);
#endif

        bool ready( ) const;

        void disconnect( );

        void assign_rpc_handler(
                          vtrc::shared_ptr<google::protobuf::Service> handler);
        void assign_weak_rpc_handler(
                          vtrc::weak_ptr<google::protobuf::Service> handler);

        vtrc::shared_ptr<google::protobuf::Service> get_rpc_handler(
                                                    const std::string &name );
        void erase_rpc_handler(
                          vtrc::shared_ptr<google::protobuf::Service> handler);

        void erase_rpc_handler( const std::string &name );

        /// ll_mess is IN OUT parameter
        void raw_call_local ( vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                              common::empty_closure_type done );

    };

    typedef vtrc::shared_ptr<vtrc_client> vtrc_client_sptr;
    typedef vtrc::weak_ptr<vtrc_client>   vtrc_client_wptr;

}}

#endif // VTRCCLIENT_H
