#ifndef VTRC_CALL_CONTEXT_H
#define VTRC_CALL_CONTEXT_H

namespace vtrc_rpc_lowlevel {
    class lowlevel_unit;
    class options;
}

namespace vtrc { namespace common {

    class call_context {

        struct impl;
        impl  *impl_;


    public:

        call_context( const call_context &other );
        call_context &operator = ( const call_context &other );

        call_context( vtrc_rpc_lowlevel::lowlevel_unit *lowlevel );

        const vtrc_rpc_lowlevel::lowlevel_unit *get_lowlevel_message( ) const;
        vtrc_rpc_lowlevel::lowlevel_unit *get_lowlevel_message( );

        const vtrc_rpc_lowlevel::options *get_call_options( ) const;
        void set_call_options(const vtrc_rpc_lowlevel::options &opts);

        virtual ~call_context( );

    };

}}

#endif // VTRCCALLCONTEXT_H
