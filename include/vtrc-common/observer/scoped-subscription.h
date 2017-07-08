#ifndef VTRC_COMMON_OBSERVERS_SCOPED_SUBSCRIPTION_H
#define VTRC_COMMON_OBSERVERS_SCOPED_SUBSCRIPTION_H

#include "config/vtrc-memory.h"
#include "subscription.h"

namespace vtrc { namespace common { namespace observer {

    class scoped_subscription {

        typedef subscription::unsubscriber_sptr unsubscriber_sptr;

    public:

        /// C-tors
#if !VTRC_DISABLE_CXX11

        scoped_subscription( scoped_subscription &&o )
        {
            unsubscriber_.swap(o.unsubscriber_);
            o.reset( );
        }

        scoped_subscription &operator = ( scoped_subscription &&o )
        {
            if( this != &o ) {
                unsubscriber_.swap(o.unsubscriber_);
                o.reset( );
            }
            return *this;
        }

        scoped_subscription( subscription &&o )
            :unsubscriber_(std::move(o.unsubscriber_))
        { }

        scoped_subscription & operator = ( subscription &&o )
        {
            unsubscribe( );
            unsubscriber_.swap(o.unsubscriber_);
            return *this;
        }
#endif
        scoped_subscription( scoped_subscription &o )
        {
            unsubscriber_.swap( o.unsubscriber_ );
            o.reset( );
        }

        scoped_subscription( )
        { }

        scoped_subscription( const subscription &o )
            :unsubscriber_(o.unsubscriber_)
        { }

        /// D-tor
        ~scoped_subscription( )
        {
            unsubscribe( );
        }

        /// O-tor
        scoped_subscription &operator = ( scoped_subscription &o )
        {
            if( this != &o ) {
                unsubscriber_.swap( o.unsubscriber_ );
                o.reset( );
            }
            return *this;
        }

        scoped_subscription &operator = ( const subscription &o )
        {
            unsubscribe( );
            unsubscriber_ = o.unsubscriber_;
            return *this;
        }

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

        subscription release( )
        {
            subscription tmp;
            unsubscriber_.swap( tmp.unsubscriber_ );
            return tmp;
        }

        void reset( )
        {
            if( unsubscriber_ ) {
                unsubscriber_sptr tmp;
                unsubscriber_.swap(tmp);
                tmp->run( );
            }
        }

        void swap( scoped_subscription &other )
        {
            unsubscriber_.swap( other.unsubscriber_ );
        }

    private:
        unsubscriber_sptr unsubscriber_;
    };

    typedef scoped_subscription scoped_connection;

}}}

#endif // SCOPEDSUBSCRIPTION_H
