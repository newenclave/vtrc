#ifndef VTRC_SIZEPACK_POLICY_H
#define VTRC_SIZEPACK_POLICY_H

#include <string>

#include <limits>
#include <boost/static_assert.hpp>

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

template <typename SizeType>
struct varint_packer_policy {

private:
    BOOST_STATIC_ASSERT(
        !std::numeric_limits<SizeType>::is_signed
    );
public:

    typedef SizeType size_type;

    static const size_t max_length = (sizeof(SizeType) << 3) / 7 + 1;

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
        for( ;size > 0x7F; size >>= 7 ) {
            res.push_back(static_cast<char>(size & 0x7F) | 0x80);
        }
        res.push_back(static_cast<char>(size));
        return res;
    }

    template <typename IterT>
    static size_type unpack( IterT begin, const IterT &end )
    {
        size_type     res   = 0x00;
        unsigned      shift = 0x00;
        unsigned char last  = 0x80;

        for( ;(begin != end) && (last & 0x80); ++begin, shift += 7 ) {
            last = (*begin);
            res |= ( static_cast<size_type>(last & 0x7F) << shift);
        }
        return res;
    }
};

#endif
