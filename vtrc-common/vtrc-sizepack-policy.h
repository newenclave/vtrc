#ifndef VTRC_SIZEPACK_POLICY_H
#define VTRC_SIZEPACK_POLICY_H

#include <string>

#include <limits> 
#include <boost/static_assert.hpp>

template <typename SizeType>
struct packer_policy {

private:
    BOOST_STATIC_ASSERT( 
        !std::numeric_limits<SizeType>::is_signed 
    );
public:

    typedef SizeType size_type;

    static const size_t max_length = (sizeof(SizeType) << 3) / 7 + 1;

    template <typename IterT>
    static bool enough_for_size( IterT begin, const IterT &end )
    {
        for( ;begin!=end && !(*begin & 0x80); ++begin );
        return begin!=end;
    }

    static size_t packed_size( size_t input )
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
            char next = static_cast<char>(size & 0x7F) | 0x80;
            res.push_back(next);
        }
        res.push_back(static_cast<char>(size));
        return res;
    }

    //static size_type unpack( )

};

#endif