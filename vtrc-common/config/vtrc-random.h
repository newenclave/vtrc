#ifndef VTRC_RANDOM_H
#define VTRC_RANDOM_H

#include "boost/random/random_device.hpp"
#include "boost/random.hpp"


namespace vtrc {
    namespace random {
        using namespace boost::random;
    }
    using boost::random_device;
    using boost::mt19937;
}

#endif // VTRC_RANDOM_H
