#ifndef VTRC_CONDITIONAL_VARIABLE_H
#define VTRC_CONDITIONAL_VARIABLE_H

#include "vtrc-general-config.h"

#if VTRC_DISABLE_CXX11

#include "boost/thread/condition_variable.hpp"

namespace vtrc {
    using boost::condition_variable;
}

#else

#include <condition_variable>

namespace vtrc {
    using std::condition_variable;
}

#endif

#endif // VTRCCONDITIONALVARIABLE_H
