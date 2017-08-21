
#include "vtrc/common/hash/iface.h"

namespace vtrc { namespace common {  namespace hash {

    namespace {

        struct hasher_none: public iface {

            size_t size( ) const
            {
                return 0;
            }

            std::string get_data_hash( const void * /* data   */,
                                       size_t       /* length */ ) const
            {
                return std::string( );
            }

            bool check_data_hash( const void *  /* data  */,
                                  size_t        /* length*/,
                                  const void *  /* hash  */ ) const
            {
                return true;
            }

            void get_data_hash( const void * /*data*/,
                                size_t       /*length*/,
                                void       * /*result_hash*/ ) const
            {

            }

        };

    }

    namespace none {
        iface *create( )
        {
            return new hasher_none;
        }
    }

}}}
