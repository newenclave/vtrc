#ifndef VTRC_SIZEPACK_POLICY_H
#define VTRC_SIZEPACK_POLICY_H

#include <string>
#include <limits>

#include "boost/static_assert.hpp"

/*
size_packer_policy {

    type    size_type
    size_t  max_length;

    template <typename IterT>
    static size_t size_length( IterT begin, const IterT &end );

    template <typename IterT>
    static size_type unpack( IterT begin, const IterT &end )

    static size_t packed_length( size_type input )
    static std::string pack( size_type size )

}
*/

namespace vtrc { namespace common { namespace policies {

    template <typename SizeType>
    struct fixint_policy {
    private:
        BOOST_STATIC_ASSERT(
            !std::numeric_limits<SizeType>::is_signed
        );
    public:

        typedef SizeType size_type;

        static const size_t max_length = sizeof(SizeType);
        static const size_t min_length = sizeof(SizeType);

        template <typename IterT>
        static size_t size_length( IterT begin, const IterT &end )
        {
            return std::distance(begin, end) < max_length ? 0 : max_length;
        }

        static size_t packed_length( size_type input )
        {
            return max_length;
        }

        static std::string pack( size_type size )
        {
            char res[max_length];
            for( size_t current =  max_length; current > 0; --current ) {
                res[current-1] = static_cast<char>( size & 0xFF );
                size >>= 8;
            }
            return std::string( &res[0], &res[max_length] );
        }

        static size_t pack( size_type size, void *result )
        {
            uint8_t *res = reinterpret_cast<uint8_t *>(result);
            for( size_t current = max_length; current > 0; --current ) {
                res[current-1] = static_cast<uint8_t>( size & 0xFF );
                size >>= 8;
            }
            return max_length;
        }

        template <typename IterT>
        static size_type unpack( IterT begin, const IterT &end )
        {
            size_type  res   = 0x00;
            for(size_t cur=max_length; cur>0 && begin!=end; --cur, ++begin) {
                res |= static_cast<size_type>(
                            static_cast<unsigned char>(*begin))
                                            << ((cur - 1) << 3);
            }
            return res;
        }
    };

    template <typename SizeType>
    struct varint_policy {

    private:
        BOOST_STATIC_ASSERT(
            !std::numeric_limits<SizeType>::is_signed
        );
    public:

        typedef SizeType size_type;

        static const size_t max_length = (sizeof(SizeType) << 3) / 7 + 1;
        static const size_t min_length = 1;

        template <typename IterT>
        static size_t size_length( IterT begin, const IterT &end )
        {
            size_t        length = 0x00;
            unsigned char last   = 0x80;

            for( ;(begin != end) && (last & 0x80); ++begin ) {
                ++length;
                last = static_cast<unsigned char>(*begin);
            }
            return (last & 0x80) ? 0 : length;
        }

        static size_t packed_length( size_type input )
        {
            size_t res = 0;
            while( input ) ++res, input >>= 7;
            return res;
        }

        static std::string pack( size_type size )
        {
            std::string res;
            res.reserve(sizeof(size_type));
            for( ; size > 0x7F; size >>= 7 ) {
                res.push_back(static_cast<char>(size & 0x7F) | 0x80);
            }
            res.push_back(static_cast<char>(size));
            return res;
        }

        static size_t pack( size_type size, void *result )
        {
            size_t   index = 0;
            uint8_t *res = reinterpret_cast<uint8_t *>(result);
            for( ; size > 0x7F; size >>= 7 ) {
                res[index++] = (static_cast<char>(size & 0x7F) | 0x80);
            }
            res[index++] = (static_cast<char>(size));
            return index;
        }

        template <typename IterT>
        static size_type unpack( IterT begin, const IterT &end )
        {
            size_type     res   = 0x00;
            unsigned      shift = 0x00;
            unsigned char last  = 0x80;

            for( ; (begin != end) && (last & 0x80); ++begin, shift += 7 ) {
                last = (*begin);
                res |= ( static_cast<size_type>(last & 0x7F) << shift);
            }
            return res;
        }
    };

}}}

#endif
