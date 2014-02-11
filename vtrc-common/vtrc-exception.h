#ifndef VTRC_ERRORS_H
#define VTRC_ERRORS_H

#include <exception>

namespace vtrc_errors {
    class error_container;
}

namespace vtrc { namespace common {

    class exception: public std::exception {
        std::string what_;
        std::string additional_;
    public:
        exception( unsigned code, const std::string &additional );
        exception( unsigned code );
        const char *what( ) const;
        const char *additional( ) const;
        unsigned   code( ) const;
    };

}}

#endif // VTRCERRORS_H
