#ifndef VTRC_CALL_CONTEXT_H
#define VTRC_CALL_CONTEXT_H

#include "vtrc/common/connection-iface.h"
#include "vtrc/common/config/vtrc-cxx11.h"

namespace vtrc { namespace rpc {
    class lowlevel_unit;
    class options;
}}

namespace google { namespace protobuf {
    class Closure;
}}

namespace vtrc { namespace common {

    class call_context {

        struct impl;
        impl  *impl_;

    public:
        /// get context
        static const call_context *get(  );

    public:

        call_context( rpc::lowlevel_unit *lowlevel );
        virtual ~call_context( );

        call_context( const call_context &other );
        call_context &operator = ( const call_context &other );

        call_context       *next( );
        const call_context *next( ) const;

        void set_next( call_context *next );

        rpc::lowlevel_unit       *get_lowlevel_message( );
        const rpc::lowlevel_unit *get_lowlevel_message( ) const;

        void set_impersonated( bool value );
        bool get_impersonated( ) const;

        const rpc::options *get_call_options( ) const;
        void set_call_options(const rpc::options *opts);

        void set_done_closure( google::protobuf::Closure *done );

        google::protobuf::Closure       *get_done_closure(  );
        const google::protobuf::Closure *get_done_closure(  ) const;

        size_t depth( ) const;

        const std::string &channel_data( ) const;

    };

    typedef vtrc::shared_ptr<call_context> call_context_sptr;
}}

#endif // VTRCCALLCONTEXT_H
