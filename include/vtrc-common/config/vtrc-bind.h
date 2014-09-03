#ifndef VTRC_BIND_H
#define VTRC_BIND_H

#include "vtrc-general-config.h"

#if VTRC_DISABLE_CXX11

#include "boost/bind.hpp"

namespace vtrc {

    using boost::bind;

    namespace placeholders {
        namespace {
            boost::arg<1> _1;
            boost::arg<2> _2;
            boost::arg<3> _3;
            boost::arg<4> _4;
            boost::arg<5> _5;
            boost::arg<6> _6;
            boost::arg<7> _7;
            boost::arg<8> _8;
            boost::arg<9> _9;
        }

        static boost::arg<1> &error             = _1;
        static boost::arg<2> &bytes_transferred = _2;
//        static boost::arg<2> &iterator          = _2;
//        static boost::arg<2> &signal_number     = _2;
    }
}
#else

#ifdef _MSC_VER
#define _VARIADIC_MAX 9
#endif

#include <functional>

namespace vtrc {

    using std::bind;

    namespace placeholders {

        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        using std::placeholders::_4;
        using std::placeholders::_5;
        using std::placeholders::_6;
        using std::placeholders::_7;
        using std::placeholders::_8;
        using std::placeholders::_9;

        static decltype( _1 ) &error             = _1;
        static decltype( _2 ) &bytes_transferred = _2;

        //static decltype( _2 ) &iterator          = _2;
        //static decltype( _2 ) &signal_number     = _2;
    }
}

#endif
#endif // VTRCBIND_H
