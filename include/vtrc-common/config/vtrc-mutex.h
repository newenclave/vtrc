#ifndef VTRC_MUTEX_H
#define VTRC_MUTEX_H

#include "vtrc-general-config.h"

#if VTRC_DISABLE_CXX11

#include "boost/thread/mutex.hpp"
#include "boost/thread/locks.hpp"

namespace vtrc {
    using boost::mutex;
    using boost::unique_lock;
    using boost::lock_guard;
}

#else

#include <mutex>

namespace vtrc {
    using std::mutex;
    using std::unique_lock;
    using std::lock_guard;
}

#endif

#endif // VTRC_MUTEX_H
