#include "vtrc-hasher-iface.h"

namespace vtrc { namespace common {  namespace hasher { 
	
	namespace {
		
		struct hasher_fake: public  hasher_iface{

			size_t hash_size( ) const 
			{
				return 0;
			}
			
			std::string get_data_hash(const void * /*data*/, size_t /*length*/) const
			{
				static const std::string fh;
				return fh;
			}
			
			bool check_data_hash( const void * /*data*/, 
							 size_t /*length*/,
							 const void * /*hash*/) const
			{
				return true;
			}
		};
		
	}
	
	namespace  fake {
		hasher_iface *create( )
		{
			return new hasher_fake;
		}
	}

}}}