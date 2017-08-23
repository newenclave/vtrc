#ifndef VTRC_CONNECTION_LIST_H
#define VTRC_CONNECTION_LIST_H

#include "vtrc-memory.h"
#include "vtrc/common/connection-iface.h"

namespace vtrc { namespace common {

    class connection_list: public enable_shared_from_this<connection_list> {

        struct impl;
        impl  *impl_;

        connection_list( );
        VTRC_DISABLE_COPY( connection_list );

    public:

        typedef vtrc::shared_ptr<connection_list> sptr;

        typedef vtrc::function <
                bool (common::connection_iface_sptr)
        > client_predic;

        static vtrc::shared_ptr<connection_list> create( );

        vtrc::weak_ptr<connection_list> weak_from_this( )
        {
            return vtrc::weak_ptr<connection_list>( shared_from_this( ) );
        }

        vtrc::weak_ptr<connection_list const> weak_from_this( ) const
        {
            return vtrc::weak_ptr<connection_list const>( shared_from_this( ) );
        }

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
