#ifndef VTRC_CALL_KEEPER_H
#define VTRC_CALL_KEEPER_H

#include "vtrc-connection-iface.h"
#include "vtrc-memory.h"

namespace vtrc { namespace common {

    class call_keeper {

        struct  impl;
        impl   *impl_;

        call_keeper( const call_keeper &other );
        call_keeper& operator = ( const call_keeper &other );

    public:

        call_keeper( connection_iface *connection );
        virtual ~call_keeper( );

    public:

        static
        vtrc::shared_ptr<call_keeper> create( connection_iface *connection );
        const call_context *get_call_context( ) const;


    public:

        bool valid( ) const;
    };

    typedef vtrc::shared_ptr<call_keeper> call_keeper_sptr;

}}

#endif // VTRC_CALL_KEEPER_H
