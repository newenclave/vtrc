#ifndef VTRC_CLIENT_H
#define VTRC_CLIENT_H

#include "vtrc-general-config.h"

#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-lowlevel-protocol-iface.h"
#include "vtrc-common/vtrc-protocol-iface.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"

#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-rpc-channel.h"

#include "vtrc-memory.h"
#include "vtrc-function.h"
#include "vtrc-asio-forward.h"
#include "vtrc-system.h"
#include "vtrc-client-base.h"

VTRC_ASIO_FORWARD( class io_service; )

#if VTRC_OPENSSL_ENABLED
VTRC_ASIO_FORWARD(
    namespace ssl {
        class verify_context;
    }
)
#endif


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

    typedef base::service_factory_type  service_factory_type;
    typedef base::lowlevel_factory_type lowlevel_factory_type;

#if VTRC_OPENSSL_ENABLED
    typedef vtrc::function<
        bool ( bool, VTRC_ASIO::ssl::verify_context& )
    > verify_callback_type;
#endif

    class vtrc_client: public vtrc::client::base {

        struct        impl;
        friend struct impl;
        impl         *impl_;

    protected:

        vtrc_client( VTRC_ASIO::io_service &ios );

        vtrc_client( VTRC_ASIO::io_service &ios,
                     VTRC_ASIO::io_service &rpc_ios );

        vtrc_client( common::pool_pair &pools );

    public:

        ~vtrc_client( );

        static
        vtrc::shared_ptr<vtrc_client> create( VTRC_ASIO::io_service &ios );

        static
        vtrc::shared_ptr<vtrc_client> create( VTRC_ASIO::io_service &ios,
                                              VTRC_ASIO::io_service &rpc_ios );
        static
        vtrc::shared_ptr<vtrc_client> create( common::pool_pair &pool );

    public:

        void set_session_key( const std::string &id, const std::string &key );
        void set_session_key( const std::string &key );

        std::string get_session_key(  ) const;

        void  reset_session_key( );
        void  reset_session_info( ); // key and id

        static const common::call_context *get_call_context( );

    public:

        template <typename Conn>
        vtrc::shared_ptr<Conn> assign(  )
        {
            vtrc::shared_ptr<Conn> n = Conn::create(  );
            n->set_protocol( init_protocol( n ) );
            return n;
        }

#if VTRC_DISABLE_CXX11
        template <typename Conn, typename T0>
        vtrc::shared_ptr<Conn> assign( T0 &t0 )
        {
            vtrc::shared_ptr<Conn> n = Conn::create( t0 );
            n->set_protocol( init_protocol( n ) );
            return n;
        }

        template <typename Conn, typename T0, typename T1>
        vtrc::shared_ptr<Conn> assign( T0 &t0,
                                       const T1 &t1 )
        {
            vtrc::shared_ptr<Conn> n = Conn::create( t0, t1 );
            n->set_protocol( init_protocol( n ) );
            return n;
        }

        template <typename Conn, typename T0, typename T1, typename T2>
        vtrc::shared_ptr<Conn> assign( T0 &t0,
                                       const T1 &t1,
                                       const T2 &t2 )
        {
            vtrc::shared_ptr<Conn> n = Conn::create( t0, t1, t2 );
            n->set_protocol( init_protocol( n ) );
            return n;
        }
#else
        template <typename Conn, typename ...Args >
        vtrc::shared_ptr<Conn> assign( Args && ... args )
        {
            vtrc::shared_ptr<Conn> n =
                    Conn::create( std::forward<Args>( args )... );
            n->set_protocol( init_protocol( n ) );
            return n;
        }
#endif

    public:

#if VTRC_OPENSSL_ENABLED
        void connect_ssl( const std::string &address,
                          unsigned short service,
                          const std::string &verify_file,
                          verify_callback_type cb,
                          bool tcp_nodelay = false );

        void connect_ssl( const std::string &address,
                          unsigned short service,
                          const std::string &verify_file,
                          bool tcp_nodelay = false );

        void async_connect_ssl( const std::string &address,
                                unsigned short     service,
                                common::system_closure_type closure,
                                const std::string &verify_file,
                                verify_callback_type cb,
                                bool tcp_nodelay = false );

        void async_connect_ssl( const std::string &address,
                                unsigned short     service,
                                common::system_closure_type closure,
                                const std::string &verify_file,
                                bool tcp_nodelay = false );
#endif

#ifndef _WIN32
        void open( const std::string &path, int flags, int mode );
#endif

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

        void connect( );
        void async_connect( common::system_closure_type );

    };

    typedef vtrc::shared_ptr<vtrc_client> vtrc_client_sptr;
    typedef vtrc::weak_ptr<vtrc_client>   vtrc_client_wptr;

}}

#endif // VTRCCLIENT_H
