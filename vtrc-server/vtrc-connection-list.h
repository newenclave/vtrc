#ifndef VTRC_CONNECTION_LIST_H
#define VTRC_CONNECTION_LIST_H

#include "vtrc-common/vtrc-connection-iface.h"

namespace vtrc {

    namespace common {
        struct connection_iface;
    }

namespace server {

    class connection_list {

        struct impl;
        impl  *impl_;

    public:

        typedef boost::function <
                bool (common::connection_iface_sptr)
        > client_predic;

        connection_list( );
        ~connection_list( );

        void clear( );

        void store( common::connection_iface *connection );
        void store( common::connection_iface_sptr connection );

        void drop ( common::connection_iface *connection );
        void drop ( common::connection_iface_sptr connection );

        size_t foreach_while(client_predic func);

    };
}}

#endif // VTRC_CONNECTION_LIST_H
