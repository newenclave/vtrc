
#include "vtrc-random-device.h"

#include <boost/random/random_device.hpp>
#include <boost/random.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <limits>

namespace  vtrc { namespace  common {

    struct random_impl {
        virtual ~random_impl( );
        virtual void generate( char *b, char *e ) = 0;
    };

    struct device_impl: public random_impl {
        boost::random_device rd_;
        void generate( char *b, char *e )
        {
            rd_.generate( b, e );
        }
    };

    struct pseudo_impl: public random_impl {

        boost::mt19937 rng_;
        pseudo_impl(  )
        {
            boost::random_device rd;
            rng_.seed( rd( ) );
        }

        void generate( char *b, char *e )
        {
            boost::random::uniform_int_distribution< > index_dist(0, UCHAR_MAX);
            for( ;b!=e ;++b ) {
                *b = index_dist(rng_);
            }
        }
    };

    struct random_device::impl {
        boost::shared_ptr<random_impl> random_dev_;
        impl( bool use_mt19937 )
        {
            if( !use_mt19937 ) {
                random_dev_ = boost::make_shared<device_impl>( );
            } else {
                random_dev_ = boost::make_shared<pseudo_impl>( );
            }
        }
        void generate(char *b, char *e)
        {
            random_dev_->generate( b, e );
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

}}



