#ifndef VTRC_RANDOM_DEVICE_H
#define VTRC_RANDOM_DEVICE_H

#include <string>

namespace vtrc { namespace common {

    class random_device {

        struct impl;
        impl  *impl_;

        random_device( const random_device &);
        random_device &operator = ( const random_device &);

    public:

        random_device( bool use_mt19937 = false );
        ~random_device( );

        void generate( char *b, char *e );

        std::string generate_block( size_t length );

    };

}}

#endif // VTRCRANDOMDEVICE_H
