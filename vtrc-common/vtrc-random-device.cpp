
#include "vtrc-random-device.h"

#include <boost/random/random_device.hpp>
#include <boost/random.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace  vtrc { namespace  common {

    struct random_impl {
        template <typename IterType>
        virtual void generate( IterType b, const IterType e ) = 0;
    };

    struct device_impl: public random_impl {
        boost::random_device rd_;
        template <typename IterType>
        void generate( IterType b, const IterType e )
        {
            rd_.generate( b, e );
        }
    };

    struct pseudo_impl: public random_impl {

        boost::mt19937 rng_;
        random_impl(  )
        {
            boost::random_device rd;
            unsigned seed = 0;
            rd.generate( reinterpret_cast<char *>(&seed),
                         reinterpret_cast<char *>(&seed) + sizeof(seed));
            rng_.seed( seed );
        }

        template <typename IterType>
        void generate( IterType b, const IterType e )
        {

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
    };

    random_device::random_device( bool use_device )
        :impl_(new impl(use_device))
    {

    }

    random_device::~random_device( )
    {
        delete impl_;
    }

}}



