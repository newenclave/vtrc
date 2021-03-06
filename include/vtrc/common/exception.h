#ifndef VTRC_ERRORS_H
#define VTRC_ERRORS_H

#include <stdexcept>
#include <string>
#include "vtrc/common/noexcept.h"
#include "vtrc/common/config/vtrc-cxx11.h"

//namespace vtrc { namespace rpc { namespace errors {
//    class error_container;
//}}}

namespace vtrc { namespace common {

    template <typename T>
    void raise( const T &ex )
    {
        throw ex;
    }

    class exception: public std::runtime_error {

        unsigned    code_;
        unsigned    category_;

        //std::string what_;
        std::string additional_;

    public:

        explicit exception( unsigned code ); // internal category

        exception( unsigned code, unsigned code_category );
        exception( unsigned code,
                   unsigned code_category,
                   const std::string &additional );
        exception( unsigned code, const std::string &additional );

        virtual ~exception( );

        const char *additional( ) const;
        unsigned    code( )       const;
        unsigned    category( )   const;
    };

    void throw_protocol_error( unsigned code );
    void throw_system_error( unsigned code );
    void throw_system_error( unsigned code, const std::string &additional );
    std::string error_code_to_string( unsigned code, unsigned category );

}}

#endif // VTRCERRORS_H
