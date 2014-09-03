#ifndef VTRC_RANDOM_H
#define VTRC_RANDOM_H

#include "vtrc-general-config.h"


#if VTRC_DISABLE_CXX11

#include "boost/random/random_device.hpp"
#include "boost/random.hpp"

namespace vtrc {
    namespace random {
        using namespace boost::random;
    }
    using boost::random_device;
    using boost::mt19937;
}


#else

#include <random>

namespace vtrc {
    namespace random {
        using std::uniform_int_distribution;
    }
    using std::random_device;
    using std::mt19937;
}

#endif

#endif // VTRC_RANDOM_H
