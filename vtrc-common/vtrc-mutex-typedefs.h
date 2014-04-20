#ifndef VTRC_MUTEX_TYPEDEFS_H
#define VTRC_MUTEX_TYPEDEFS_H

#include "boost/thread/mutex.hpp"
#include "boost/thread/shared_mutex.hpp"

namespace vtrc {
    typedef boost::shared_mutex                          shared_mutex;
    typedef boost::unique_lock<shared_mutex>             unique_shared_lock;
    typedef boost::shared_lock<shared_mutex>             shared_lock;
    typedef boost::upgrade_lock<shared_mutex>            upgradable_lock;
    typedef boost::upgrade_to_unique_lock<shared_mutex>  upgrade_to_unique;
}

#endif // VTRC_MUTEX_TYPEDEFS_H
