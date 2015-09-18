
#include <algorithm>

#include "google/protobuf/descriptor.h"
#include "boost/system/error_code.hpp"

#include "vtrc-exception.h"
#include "vtrc-errors.pb.h"

namespace vtrc { namespace common {

    namespace gpb = google::protobuf;

    namespace {

        const std::string unknown_error("Unknown error");
        const unsigned default_category = rpc::errors::CATEGORY_INTERNAL;

        const std::string &get_internal_error( unsigned code )
        {
            const gpb::EnumValueDescriptor *ev =
                rpc::errors::errors_numbers_descriptor( )
                                ->FindValueByNumber( code );
            if( ev ) {
                return ev->options( ).GetExtension( rpc::errors::description );
            }
            return unknown_error;
        }

        std::string get_error_code_string( unsigned code, unsigned category )
        {
            if( category == rpc::errors::CATEGORY_INTERNAL ) {
                return get_internal_error( code );
            } else if( category == rpc::errors::CATEGORY_SYSTEM ) {
                try {
                    boost::system::error_code ec(code,
                                             boost::system::system_category( ));
                    return ec.message( );
                } catch( const std::exception &ex ) {
                    return ex.what( );
                }
            }
            return unknown_error;
        }

        std::string get_error_code_string( unsigned code )
        {
            return get_error_code_string( code, default_category );
        }

    }

    exception::exception( unsigned code, unsigned code_category )
        :std::runtime_error(get_error_code_string(code, code_category))
        ,code_(code)
        ,category_(code_category)
    { }

    exception::exception(unsigned code,
                         unsigned code_category,
                         const std::string &additional)
        :std::runtime_error(get_error_code_string(code, code_category))
        ,code_(code)
        ,category_(code_category)
        ,additional_(additional)
    { }

    exception::exception( unsigned code )
        :std::runtime_error(get_error_code_string(code))
        ,code_(code)
        ,category_(default_category)
    { }

    exception::exception( unsigned code, const std::string &additional )
        :std::runtime_error(get_error_code_string(code))
        ,code_(code)
        ,category_(default_category)
        ,additional_(additional)
    { }

    exception::~exception( ) throw ( )
    { }

    const char *exception::additional( ) const
    {
        return additional_.c_str( );
    }

    unsigned exception::code( ) const
    {
        return code_;
    }

    unsigned exception::category( ) const
    {
        return category_;
    }

    void throw_system_error( unsigned code )
    {
        throw exception( code, rpc::errors::CATEGORY_SYSTEM );
    }

    void throw_system_error( unsigned code, const std::string &additional )
    {
        throw exception( code, rpc::errors::CATEGORY_SYSTEM, additional );
    }

    std::string error_code_to_string( unsigned code, unsigned category )
    {
        return get_error_code_string( code, category );
    }

}}
