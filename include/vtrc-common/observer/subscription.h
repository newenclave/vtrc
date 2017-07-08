#ifndef VTRC_COMMON_OBSERVERS_SUBSCRIPTION_H
#define VTRC_COMMON_OBSERVERS_SUBSCRIPTION_H

#include "config/vtrc-memory.h"
#include "config/vtrc-stdint.h"
#include "scoped-subscription.h"

namespace vtrc { namespace common { namespace observer {

    class scoped_subscription;
    class subscription {

        friend class vtrc::common::observer::scoped_subscription;

        static void unsubscribe_dummy( )
        { }

        void reset( )
        {
            unsubscriber_.reset( );
        }

    public:

        struct unsubscriber {
            virtual vtrc::uintptr_t data( ) { return 0; }
            virtual ~unsubscriber( ) { }
            virtual void run( ) = 0;
        };

        typedef vtrc::shared_ptr<unsubscriber> unsubscriber_sptr;

        subscription( unsubscriber_sptr call )
            :unsubscriber_(call)
        { }

#if !VTRC_DISABLE_CXX11
        subscription( subscription &&o )
            :unsubscriber_(std::move(o.unsubscriber_))
        { }

        subscription &operator = ( subscription &&o )
        {
            unsubscriber_ = o.unsubscriber_;
            return *this;
        }

        subscription( const subscription &o ) = default;
        subscription &operator = ( const subscription &o ) = default;
#endif
        subscription( )
        { }

        void unsubscribe(  )
        {
            if( unsubscriber_ ) {
                unsubscriber_->run( );
            }
        }

        void disconnect(  )
        {
            unsubscribe( );
        }

        void swap( subscription &other )
        {
            unsubscriber_.swap( other.unsubscriber_ );
        }
    private:
        unsubscriber_sptr unsubscriber_;
    };

    /// boost::signal compatibility
    typedef subscription connection;

}}}

#endif // SUBSCRIPTION_H
