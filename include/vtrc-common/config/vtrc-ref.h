#ifndef VTRCREF_H
#define VTRCREF_H

#include "vtrc-general-config.h"


#if VTRC_DISABLE_CXX11


#include "boost/ref.hpp"

namespace vtrc {
    using boost::ref;
    using boost::cref;
}

#else

#include <functional>

namespace vtrc {
    using std::ref;
    using std::cref;
}

#endif

#endif // VTRCREF_H
