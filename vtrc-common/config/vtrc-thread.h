#ifndef VTRC_THREAD_H
#define VTRC_THREAD_H

#include "boost/thread.hpp"

namespace vtrc {
    using boost::thread;

    namespace this_thread {
        using namespace boost::this_thread;
    }

}

#endif // VTRCTHREAD_H
