#ifndef VTRC_ERRORS_H
#define VTRC_ERRORS_H

#include <exception>

namespace vtrc { namespace common {

    class exception: public std::exception {
    public:
        exception( unsigned code, const std::string &additional );
        exception( unsigned code );
        const char what( ) const;
    };

}}

#endif // VTRCERRORS_H
