#ifndef VTRC_ATOMIC_H
#define VTRC_ATOMIC_H

#include "vtrc-general-config.h"

#if VTRC_DISABLE_CXX11

#include "boost/atomic.hpp"

namespace vtrc {
    using boost::atomic;
}

#else

#include <atomic>

namespace vtrc {
    using std::atomic;
}

#endif

#endif // VTRCATOMIC_H
