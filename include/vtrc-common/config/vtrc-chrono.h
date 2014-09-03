#ifndef VTRC_CHRONO_H
#define VTRC_CHRONO_H

#include "vtrc-general-config.h"

#if VTRC_DISABLE_CXX11

#include "boost/chrono.hpp"

namespace vtrc {
    namespace chrono {
        using namespace boost::chrono;
    }
}

#else

#include <chrono>

namespace vtrc {
    namespace chrono {
        using namespace std::chrono;
    }
}

#endif

#endif // VTRCCHRONO_H
