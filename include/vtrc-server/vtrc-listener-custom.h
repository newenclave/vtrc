#ifndef VTRC_LISTENER_CUSTOM_H
#define VTRC_LISTENER_CUSTOM_H

#include <string>

//#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-listener.h"

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

        listener_sptr create( application &app, const std::string &name );
        listener_sptr create( application &app,
                                      const rpc::session_options &opts,
                                      const std::string &name );

        ~custom( );

        std::string name( ) const;
        void start( );
        void stop ( );
        bool is_active( ) const;
        bool is_local( )  const;

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

