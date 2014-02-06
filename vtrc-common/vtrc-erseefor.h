#ifndef VTRC_ERSEEFOR_H
#define VTRC_ERSEEFOR_H

#include <string>
#include <memory.h>

namespace vtrc { namespace common { namespace crypt {

    class erseefor {

        typedef unsigned char uchar_type;

        uchar_type state_[256];
        uchar_type si2_;
        uchar_type si1_;

        static inline
        void swap_bytes(uchar_type &a, uchar_type &b)
        {
            uchar_type t;
            t = a;
            a = b;
            b = t;
        }

        erseefor( const erseefor & );
        erseefor& operator = ( const erseefor & );

    public:

        ~erseefor( ) /*throw()*/
        { }

        erseefor( )
        { }

        erseefor(const uchar_type *key, size_t keysize)
        {
            init_key( key, keysize );
        }

        void init_key( const uchar_type *key, const size_t keysize )
        {

            size_t  i;
            uchar_type k = 0;

            memset(state_, 0, sizeof(state_));

            for(i=0; i < 256; i++)
                state_[i] = static_cast<uchar_type>(i);

            si1_ = si2_ = 0;

            for ( i = 0; i < 256; i++ ) {
                k += (state_[i] + key[i % keysize]);
                swap_bytes(state_[i], state_[k]);
            }
        }

        void drop( size_t count )
        {
            size_t i;
            for( i = 0; i < count; i++ ) {
                si1_ ++;
                si2_ += state_[si1_];
                swap_bytes(state_[si1_], state_[si2_]);
            }
        }

        template <typename ItrType>
        void transform( ItrType b, const ItrType& e )
        {
            uchar_type  k = 0;

            for( ;b!=e ; ++b ) {
                si1_ ++;
                si2_ += state_[si1_];
                swap_bytes(state_[si1_], state_[si2_]);
                k = state_[si1_] + state_[si2_];
                (*b) ^= state_[k];
            }
        }
    };

}}}

#endif // VTRCERSEEFOR_H
