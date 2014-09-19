#ifndef VTRC_STDINT_H
#define VTRC_STDINT_H

#include "vtrc-general-config.h"

#ifdef _WIN32

#if _MSC_VER >= 1600

// Visual Studio 2010+
#include <cstdint>

namespace vtrc {

    using std::int8_t;
    using std::int16_t;
    using std::int32_t;
    using std::int64_t;

    using std::int_fast8_t;
    using std::int_fast16_t;
    using std::int_fast32_t;
    using std::int_fast64_t;

    using std::int_least8_t;
    using std::int_least16_t;
    using std::int_least32_t;
    using std::int_least64_t;

    using std::intmax_t;
    using std::intptr_t;

    using std::uint8_t;
    using std::uint16_t;
    using std::uint32_t;
    using std::uint64_t;

    using std::uint_fast8_t;
    using std::uint_fast16_t;
    using std::uint_fast32_t;
    using std::uint_fast64_t;

    using std::uint_least8_t;
    using std::uint_least16_t;
    using std::uint_least32_t;
    using std::uint_least64_t;

    using std::uintmax_t;
    using std::uintptr_t;
}

#else

#include <boost/cstdint.hpp>

namespace vtrc {

    using boost::int8_t;
    using boost::int_least8_t;
    using boost::int_fast8_t;
    using boost::uint8_t;
    using boost::uint_least8_t;
    using boost::uint_fast8_t;

    using boost::int16_t;
    using boost::int_least16_t;
    using boost::int_fast16_t;
    using boost::uint16_t;
    using boost::uint_least16_t;
    using boost::uint_fast16_t;

    using boost::int32_t;
    using boost::int_least32_t;
    using boost::int_fast32_t;
    using boost::uint32_t;
    using boost::uint_least32_t;
    using boost::uint_fast32_t;

#ifndef BOOST_NO_INT64_T

    using boost::int64_t;
    using boost::int_least64_t;
    using boost::int_fast64_t;
    using boost::uint64_t;
    using boost::uint_least64_t;
    using boost::uint_fast64_t;

#endif

    using boost::intmax_t;
    using boost::uintmax_t;
}

#endif

#else

#include <cstdint>

namespace vtrc {

    using std::int8_t;
    using std::int16_t;
    using std::int32_t;
    using std::int64_t;

    using std::int_fast8_t;
    using std::int_fast16_t;
    using std::int_fast32_t;
    using std::int_fast64_t;

    using std::int_least8_t;
    using std::int_least16_t;
    using std::int_least32_t;
    using std::int_least64_t;

    using std::intmax_t;
    using std::intptr_t;

    using std::uint8_t;
    using std::uint16_t;
    using std::uint32_t;
    using std::uint64_t;

    using std::uint_fast8_t;
    using std::uint_fast16_t;
    using std::uint_fast32_t;
    using std::uint_fast64_t;

    using std::uint_least8_t;
    using std::uint_least16_t;
    using std::uint_least32_t;
    using std::uint_least64_t;

    using std::uintmax_t;
    using std::uintptr_t;
}

#endif

#endif // VTRC_STDINT_H
