#ifndef VTRC_LISTENER_CUSTOM_H
#define VTRC_LISTENER_CUSTOM_H

#include <string>

//#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-listener.h"
#include "vtrc-common/vtrc-protocol-iface.h"
#include "vtrc-general-config.h"

namespace vtrc { namespace rpc {
    class session_options;
}}

namespace vtrc { namespace server {

    class application;

    namespace listeners {

    class custom: public listener {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        custom( application & app,
                const rpc::session_options &opts,
                const std::string &name );

    public:

        typedef vtrc::shared_ptr<custom> shared_type;

        static
        shared_type create( application &app, const std::string &name );

        static
        shared_type create( application &app, const rpc::session_options &opts,
                            const std::string &name );

        ~custom( );

        template <typename Conn>
        vtrc::shared_ptr<Conn> accept(  )
        {
            vtrc::shared_ptr<Conn> n = Conn::create(  );
            n->set_protocol( init_protocol( n ) );
            return n;
        }

#if VTRC_DISABLE_CXX11
        template <typename Conn, typename T0>
        vtrc::shared_ptr<Conn> accept( T0 &t0 )
        {
            vtrc::shared_ptr<Conn> n = Conn::create( t0 );
            n->set_protocol( init_protocol( n ) );
            return n;
        }

        template <typename Conn, typename T0, typename T1>
        vtrc::shared_ptr<Conn> accept( T0 &t0,
                                       const T1 &t1 )
        {
            vtrc::shared_ptr<Conn> n = Conn::create( t0, t1 );
            n->set_protocol( init_protocol( n ) );
            return n;
        }

        template <typename Conn, typename T0, typename T1, typename T2>
        vtrc::shared_ptr<Conn> accept( T0 &t0,
                                       const T1 &t1,
                                       const T2 &t2 )
        {
            vtrc::shared_ptr<Conn> n = Conn::create( t0, t1, t2 );
            n->set_protocol( init_protocol( n ) );
            return n;
        }
#else
        template <typename Conn, typename ...Args >
        vtrc::shared_ptr<Conn> accept( Args && ... args )
        {
            vtrc::shared_ptr<Conn> n = Conn::create( args... );
            n->set_protocol( init_protocol( n ) );
            return n;
        }
#endif
        std::string name( ) const;
        void start( );
        void stop ( );
        bool is_active( ) const;
        bool is_local( )  const;

        void stop_client( vtrc::common::connection_iface *con );

    private:
        common::protocol_iface *init_protocol(
                common::connection_iface_sptr conn );

    };
    } /// namespace listeners

//    namespace custom_ {

//    listener_sptr create( application &app, const std::string &name );
//        listener_sptr create( application &app,
//                              const rpc::session_options &opts,
//                              const std::string &name );
//    } }

}}

#endif // VTRCLISTENERCUSTOM_H

