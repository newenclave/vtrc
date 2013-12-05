#ifndef VTRC_PACKER_IFACE_H
#define VTRC_PACKER_IFACE_H

namespace vtrc { namespace common {
    struct packer_iface {
        virtual ~packer_iface( ) { }
        virtual const hasher_iface &hasher( ) = 0;
    };
}

#endif // VTRC_PACKER_IFACE_H

