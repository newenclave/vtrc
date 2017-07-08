#ifndef VTRC_CLOSURE_H
#define VTRC_CLOSURE_H

#include "vtrc-function.h"
#include "vtrc-system.h"

namespace google { namespace protobuf {
    class MethodDescriptor;
}}

namespace vtrc { namespace rpc {
    namespace errors {
        class container;
    }
    class lowlevel_unit;
}}

namespace vtrc { namespace common {

    struct connection_iface;

    typedef void (system_closure_sign)(const VTRC_SYSTEM::error_code &);
    typedef void (protcol_closure_sign)(const rpc::errors::container &);
    typedef void (success_closure_sign)(bool);
    typedef void (lowlevel_closure_sign)(const rpc::lowlevel_unit &);

    typedef vtrc::function<system_closure_sign>   system_closure_type;
    typedef vtrc::function<protcol_closure_sign>  protcol_closure_type;
    typedef vtrc::function<success_closure_sign>  success_closure_type;
    typedef vtrc::function<void ( )>              empty_closure_type;
    typedef vtrc::function<lowlevel_closure_sign> lowlevel_closure_type;

}}

#endif // VTRCCLOSURE_H
