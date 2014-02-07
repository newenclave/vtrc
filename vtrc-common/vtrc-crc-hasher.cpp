#include <boost/crc.hpp>
#include <memory.h>

#include "vtrc-hasher-iface.h"

namespace vtrc { namespace common {  namespace hasher {

    namespace {

        struct hasher_crc32: public hasher_iface {

            size_t hash_size( ) const
            {
                return sizeof(boost::crc_32_type::value_type);
            }

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

            bool check_data_hash( const void *  data  ,
                                  size_t        length,
                                  const void *  hash  ) const
            {
                std::string h = get_data_hash( data, length );
                return memcmp( h.c_str( ), hash,
                               hash_size( ) ) == 0;
            }
        };

        struct hasher_crc16: public hasher_iface {

            size_t hash_size( ) const
            {
                return sizeof(boost::crc_16_type::value_type);
            }

            std::string get_data_hash(const void *data,
                                      size_t      length) const
            {
                boost::crc_16_type res;
                res.process_bytes(data, length);
                boost::crc_16_type::value_type crc = res.checksum( );

                std::string result( 2, 0 );

                result[3] = (crc      ) & 0xFF;
                result[2] = (crc >> 8 ) & 0xFF;

                return result;
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
    }

    namespace crc {

        hasher_iface *create16( )
        {
            return new hasher_crc16;
        }

        hasher_iface *create32( )
        {
            return new hasher_crc32;
        }
    }


}}}
