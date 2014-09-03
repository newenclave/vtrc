#ifndef VTRC_FUNCTION_H
#define VTRC_FUNCTION_H

#include "vtrc-general-config.h"

#if VTRC_DISABLE_CXX11

#include "boost/function.hpp"

namespace vtrc {
    using boost::function;
}

#else

#include <functional>

namespace vtrc {
    using std::function;
}

#endif

#endif // VTRCFUNCTION_H
