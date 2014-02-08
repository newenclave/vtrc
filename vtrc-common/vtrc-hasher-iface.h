#ifndef VTRC_HASHER_IFACE_H
#define VTRC_HASHER_IFACE_H

#include <string>
#include <boost/shared_ptr.hpp>

namespace vtrc { namespace common {
    struct hasher_iface {
        virtual ~hasher_iface( ) { }

        virtual size_t hash_size( ) const = 0;

        virtual std::string get_data_hash(const void *data,
                                          size_t length) const = 0;

        virtual bool check_data_hash( const void *data, size_t length,
                                      const void *hash) const = 0;
    };

    typedef boost::shared_ptr<hasher_iface> hasher_iface_sptr;

}}

#endif // VTRC_HASHER_IFACE_H
