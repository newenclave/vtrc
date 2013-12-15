#ifndef VTRC_HASHER_IMPLS_H
#define VTRC_HASHER_IMPLS_H

#include <string>

namespace vtrc { namespace common { 
	
	struct hasher_iface;
	
namespace hasher { 
	namespace  fake {
		hasher_iface *create( );
	}
}

}}

#endif // VTRC_HASHER_IMPLS_H
