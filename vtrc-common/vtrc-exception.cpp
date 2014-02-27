#include <google/protobuf/descriptor.h>

#include "vtrc-exception.h"
#include "protocol/vtrc-errors.pb.h"

namespace vtrc { namespace common {

    namespace gpb = google::protobuf;

    namespace {

        const std::string unknown_error("Unknown error");

        const std::string &get_internal_error( unsigned code )
        {
            const gpb::EnumValueDescriptor *ev =
                vtrc_errors::errors_numbers_descriptor( )
                                ->FindValueByNumber( code );
            if( ev ) {
                return ev->options( )
                        .GetExtension( vtrc_errors::error_description );
            }
            return unknown_error;
        }

        std::string get_error_code_string( unsigned code, unsigned category )
        {
            if( category == vtrc_errors::CATEGORY_INTERNAL ) {
                return get_internal_error( code );
            }
            return unknown_error;
        }
    }

    exception::exception( unsigned code, unsigned category )
        :code_(code)
        ,category_(category)
        ,what_(get_error_code_string(code_, category_))
    {

    }

    exception::exception( unsigned code )
        :code_(code)
        ,category_(vtrc_errors::CATEGORY_INTERNAL)
        ,what_(get_error_code_string(code_, category_))
    {}

    exception::exception( unsigned code, const std::string &additional )
        :code_(code)
        ,category_(vtrc_errors::CATEGORY_INTERNAL)
        ,what_(get_error_code_string(code_, category_))
        ,additional_(additional)
    {}

    exception::~exception( ) throw ( )
    {}

    const char *exception::what( ) const throw ( )
    {
        return what_.c_str( );
    }

    const char *exception::additional( ) const
    {
        return additional_.c_str( );
    }

    unsigned exception::code( ) const
    {
        return code_;
    }

}}
