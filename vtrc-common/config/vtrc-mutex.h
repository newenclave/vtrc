#ifndef VTRC_MUTEX_H
#define VTRC_MUTEX_H

#include <boost/thread/mutex.hpp>

namespace vtrc {
    using boost::mutex;
    using boost::unique_lock;
    using boost::lock_guard;
}

#endif // VTRC_MUTEX_H
