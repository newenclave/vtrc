#ifndef VTRC_CALL_CONTEXT_H
#define VTRC_CALL_CONTEXT_H

namespace vtrc { namespace common {

    class call_context {

        struct impl;
        impl  *impl_;

    public:

        call_context( );
        call_context( const call_context &other );
        call_context &operator = ( const call_context &other );

        virtual ~call_context( );

    };

}}

#endif // VTRCCALLCONTEXT_H
