#ifndef VTRC_CALL_CONTEXT_H
#define VTRC_CALL_CONTEXT_H

namespace vtrc_rpc_lowlevel {
    class lowlevel_unit;
}

namespace vtrc { namespace common {

    class call_context {

        struct impl;
        impl  *impl_;

    public:

        call_context( vtrc_rpc_lowlevel::lowlevel_unit *lowlevel );
        call_context( const call_context &other );
        call_context &operator = ( const call_context &other );

        const vtrc_rpc_lowlevel::lowlevel_unit *get_lowlevel_message( ) const;
        vtrc_rpc_lowlevel::lowlevel_unit *get_lowlevel_message( );

        virtual ~call_context( );

    };

}}

#endif // VTRCCALLCONTEXT_H
