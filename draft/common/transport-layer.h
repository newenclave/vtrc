#ifndef TRANSPORT_LAYER_IFACE_H
#define TRANSPORT_LAYER_IFACE_H

#include <stdlib.h>

namespace transport {

    struct read_observer {
        virtual void update( const char *data, size_t length ) = 0;
    };

    class layer {
        struct   impl;
        impl    *impl_;
    public:
        layer( );
        virtual ~layer( );
    };

}

#endif // TRANSPORTLAYERIFACE_H
