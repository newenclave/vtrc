#ifndef VTRC_CLOSURE_H
#define VTRC_CLOSURE_H

#include "vtrc-function.h"

namespace boost { namespace system {
    class error_code;
}}

namespace vtrc_errors {
    class container;
}

namespace vtrc { namespace common {

    typedef void (system_closure_sign)(const boost::system::error_code &);
    typedef void (protcol_closure_sign)(const vtrc_errors::container &);
    typedef void (success_closure_sign)(bool);

    typedef vtrc::function<system_closure_sign>  system_closure_type;
    typedef vtrc::function<protcol_closure_sign> protcol_closure_type;
    typedef vtrc::function<success_closure_sign> success_closure_type;

}}

#endif // VTRCCLOSURE_H
