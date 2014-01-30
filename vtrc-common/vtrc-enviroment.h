#ifndef VTRC_ENVIROMENT_H
#define VTRC_ENVIROMENT_H

#include <string>

namespace vtrc { namespace common {

    class enviroment {

        struct enviroment_impl;
        enviroment_impl *impl_;

    public:

        enviroment( );
        enviroment( const enviroment &other );
        ~enviroment( );

        void set( const std::string &name, const std::string &value );
        const std::string &get( const std::string &name ) const;
    };

}}

#endif // VTRC_ENVIROMENT_H
