#ifndef VTRC_HASHER_IFACE_H
#define VTRC_HASHER_IFACE_H

#include <string>



namespace vtrc { namespace common { 
	
	struct hasher_iface;
	
namespace hasher { 
	namespace  fake {
		hasher_iface *create( );
	}
}

}}

#endif // VTRC_HASHER_IFACE_H