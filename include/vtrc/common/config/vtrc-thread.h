#ifndef VTRC_THREAD_H
#define VTRC_THREAD_H

#include "vtrc-general-config.h"

#if VTRC_DISABLE_CXX11

#include "boost/thread.hpp"

namespace vtrc {
    using boost::thread;

    namespace this_thread {
        using namespace boost::this_thread;
    }

}

#else

#include <thread>

namespace vtrc {

    using std::thread;
    namespace this_thread {
        using namespace std::this_thread;
    }
}

#endif

#endif // VTRCTHREAD_H
