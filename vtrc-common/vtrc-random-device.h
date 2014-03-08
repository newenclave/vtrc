#ifndef VTRC_RANDOM_DEVICE_H
#define VTRC_RANDOM_DEVICE_H

namespace vtrc { namespace common {

    class random_device {

        struct impl;
        impl  *impl_;

        random_device( const random_device &);
        random_device &operator = ( const random_device &);

    public:

        random_device( bool use_mt19937 = false );
        ~random_device( );

    };

}}

#endif // VTRCRANDOMDEVICE_H
