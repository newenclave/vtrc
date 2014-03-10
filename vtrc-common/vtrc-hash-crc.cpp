#include <boost/crc.hpp>
#include <memory.h>

#include "vtrc-hash-iface.h"

namespace vtrc { namespace common {  namespace hash {

    namespace {

        template <typename CRCImpl>
        struct hasher_crc_base: public hash_iface {

            size_t hash_size( ) const
            {
                return sizeof(typename CRCImpl::value_type);
            }

            bool check_data_hash( const void *  data  ,
                                  size_t        length,
                                  const void *  hash  ) const
            {
                std::string h = get_data_hash( data, length );
                return memcmp( h.c_str( ), hash,
                               hash_size( ) ) == 0;
            }
        };

        typedef boost::crc_optimal <
                64,
                0xad93d23594c935a9ULL, 0, 0,
                true, true> crc64_type;

        struct hasher_crc64: public hasher_crc_base<crc64_type> {
            std::string get_data_hash(const void *data,
                                      size_t      length) const
            {
                crc64_type res;
                res.process_bytes(data, length);
                crc64_type::value_type crc = res.checksum( );
                std::string result( 8, 0 );

                result[7] = (crc      ) & 0xFF;
                result[6] = (crc >> 8 ) & 0xFF;
                result[5] = (crc >> 16) & 0xFF;
                result[4] = (crc >> 24) & 0xFF;
                result[3] = (crc >> 32) & 0xFF;
                result[2] = (crc >> 40) & 0xFF;
                result[1] = (crc >> 48) & 0xFF;
                result[0] = (crc >> 56) & 0xFF;

                return result;
            }
        };

        struct hasher_crc32: public hasher_crc_base<boost::crc_32_type> {
            std::string get_data_hash(const void *data,
                                      size_t      length) const
            {
                boost::crc_32_type res;
                res.process_bytes(data, length);
                boost::crc_32_type::value_type crc = res.checksum( );
                std::string result( 4, 0 );

                result[3] = (crc      ) & 0xFF;
                result[2] = (crc >> 8 ) & 0xFF;
                result[1] = (crc >> 16) & 0xFF;
                result[0] = (crc >> 24) & 0xFF;

                return result;
            }
        };

        struct hasher_crc16: public hasher_crc_base<boost::crc_16_type> {

            std::string get_data_hash(const void *data,
                                      size_t      length) const
            {
                boost::crc_16_type res;
                res.process_bytes(data, length);
                boost::crc_16_type::value_type crc = res.checksum( );

                std::string result( 2, 0 );

                result[1] = (crc      ) & 0xFF;
                result[0] = (crc >> 8 ) & 0xFF;

                return result;
            }

        };
    }

    namespace crc {

        hash_iface *create16( )
        {
            return new hasher_crc16;
        }

        hash_iface *create32( )
        {
            return new hasher_crc32;
        }

        hash_iface *create64( )
        {
            return new hasher_crc64;
        }
    }


}}}
