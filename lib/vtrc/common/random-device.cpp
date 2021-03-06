
#include "vtrc/common/random-device.h"

#include "vtrc-random.h"

#include <limits.h>

#include "vtrc-memory.h"

namespace  vtrc { namespace  common {

    struct random_impl {
        virtual ~random_impl( ) { }
        virtual void generate( char *b, char *e ) = 0;
    };

    struct device_impl: public random_impl {
        vtrc::random_device rd_;
        void generate( char *b, char *e )
        {
            for( ; b!= e; ++b ) {
                *b = rd_();
            }
        }
    };

    struct mt19937_impl: public random_impl {

        vtrc::mt19937 rng_;
        mt19937_impl(  )
        {
            vtrc::random_device rd;
            rng_.seed( rd( ) );
        }

        void generate( char *b, char *e )
        {
            vtrc::random::uniform_int_distribution< > index_dist(0, UCHAR_MAX);
            for( ;b!=e ;++b ) {
                *b = index_dist(rng_);
            }
        }
    };

    struct random_device::impl {
        vtrc::unique_ptr<random_impl> random_dev_;
        impl( bool use_mt19937 )
        {
            if( !use_mt19937 ) {
                random_dev_.reset( new device_impl );
            } else {
                random_dev_.reset( new mt19937_impl );
            }
        }
        void generate(char *b, char *e)
        {
            random_dev_->generate( b, e );
        }

        void geterate_block( size_t length, std::string &out )
        {
            std::string tmp(length, 0);
            generate( &tmp[0], &tmp[0] + length );
            out.swap( tmp );
        }
    };

    random_device::random_device( bool use_device )
        :impl_(new impl(use_device))
    { }

    random_device::~random_device( )
    {
        delete impl_;
    }

    void random_device::generate(char *b, char *e)
    {
        impl_->generate( b, e );
    }

    std::string random_device::generate_block( size_t length )
    {
        std::string result;
        impl_->geterate_block( length, result );
        return result;
    }

}}



