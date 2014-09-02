#ifndef VTRC_BIND_H
#define VTRC_BIND_H

//#include "config.h"

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

#endif // VTRCBIND_H
