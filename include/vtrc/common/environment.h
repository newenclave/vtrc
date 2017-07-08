#ifndef VTRC_ENVIROMENT_H
#define VTRC_ENVIROMENT_H

#include <string>

//namespace google { namespace protobuf {
//    class Message;
//}}

namespace vtrc { namespace common {

    class environment {

        struct impl;
        impl  *impl_;

    public:

        environment( );
        environment( const environment &other );
        environment &operator = ( const environment &other );
        ~environment( );

        void set( const std::string &name, const std::string &value );
        const std::string get( const std::string &name ) const;
        const std::string get( const std::string &name,
                               const std::string &default_value ) const;

        size_t remove( const std::string &name );
        bool exists( const std::string &name ) const;
    };

}}

#endif // VTRC_ENVIROMENT_H
