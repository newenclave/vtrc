#include <iostream>
#include <boost/shared_ptr.hpp>
#include <string>

namespace vtrc {

namespace common {

    struct hasher_iface {
        virtual ~hasher_iface( ) { }
        size_t hash_size( ) const = 0;
        virtual std::string get_data_hash(const void *data, size_t length) const = 0;
        virtual bool check_data_hash( const void *data, size_t length,
                                      const void *hash) const = 0;
    };

    struct packer_iface {
        virtual ~packer_iface( ) { }
        virtual const hasher_iface &hasher( ) = 0;
    };
}

namespace server {

    struct endpoint_iface;

    struct protocol_info_iface {
        virtual ~protocol_info_iface( ) { }
        virtual common::packer_iface &packer( ) = 0;
    };

    struct connection_info_iface {
        virtual ~connection_info_iface( ) { }
        virtual endpoint_iface &endpoint( ) = 0;
        virtual void close( ) = 0;
    };

    struct application_iface {
        virtual ~application_iface( ) { }
    };

    struct endpoint_iface {

        virtual ~endpoint_iface( ) { }
        virtual application_iface &application( ) = 0;

        virtual std::string str( ) const = 0;

        virtual void init ( ) = 0;
        virtual void start( ) = 0;
        virtual void stop ( ) = 0;
    };
}}



int main( )
{
    return 0;
}


