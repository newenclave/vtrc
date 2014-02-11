#ifndef VTRC_ERRORS_H
#define VTRC_ERRORS_H

#include <exception>

namespace vtrc { namespace common {

    class exception: public std::exception {
    public:
        const char what( ) const;
    };

}}

#endif // VTRCERRORS_H
