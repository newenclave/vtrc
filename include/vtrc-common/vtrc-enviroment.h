#ifndef VTRC_ENVIROMENT_H
#define VTRC_ENVIROMENT_H

#include <string>

//namespace google { namespace protobuf {
//    class Message;
//}}

namespace vtrc { namespace common {

    class enviroment {

        struct impl;
        impl  *impl_;

    public:

        enviroment( );
        enviroment( const enviroment &other );
        enviroment &operator = ( const enviroment &other );
        ~enviroment( );

        void set( const std::string &name, const std::string &value );
        const std::string get( const std::string &name ) const;
        const std::string get( const std::string &name,
                               const std::string &default_value ) const;
    };

}}

#endif // VTRC_ENVIROMENT_H
