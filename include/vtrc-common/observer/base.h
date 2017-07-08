#ifndef VTRC_COMMON_OBSERVERS_COMMON_H
#define VTRC_COMMON_OBSERVERS_COMMON_H

#include <set>

#include "vtrc-mutex.h"
#include "vtrc-memory.h"
#include "details/list.h"
#include "traits/simple.h"

#include "scoped-subscription.h"
#include "subscription.h"

namespace vtrc { namespace common { namespace observer {

    template <typename SlotType, typename MutexType>
    class base {

        typedef base<SlotType, MutexType> this_type;

    public:

        typedef SlotType slot_traits;
        typedef typename SlotType::value_type  slot_type;

    private:

        typedef MutexType                     mutex_type;
        typedef vtrc::lock_guard<mutex_type>  guard_type;

        struct impl {

            typedef base<SlotType, MutexType> parent_type;

            struct slot_info {
                slot_info( const slot_type &slot, size_t id )
                    :slot_(slot)
                    ,id_(id)
                { }
                slot_type slot_;
                size_t    id_;
            };

            /// NOT STL LIST!
            typedef details::list<slot_info>      list_type;
            typedef typename list_type::iterator  list_iterator;
            typedef std::set<size_t>              iterator_set;

            iterator_set        removed_;
            list_type           list_;
            list_type           added_;

            mutable mutex_type  list_lock_;
            mutable mutex_type  tmp_lock_;
            size_t              id_;

            impl( )
                :id_(1)
            { }

            ~impl( )
            {
                clear_unsafe( );
                //clear( ); // ??? hm
            }

            static
            list_iterator itr_erase( list_type &lst, list_iterator itr )
            {
                slot_traits::erase( itr->slot_ );
                return lst.erase( itr );
            }

            static
            list_iterator itr_rerase( list_type &lst, list_iterator itr )
            {
                slot_traits::erase( itr->slot_ );
                return lst.rerase( itr );
            }

            void unsubscribe( size_t itr )
            {
                guard_type lck(tmp_lock_);
                removed_.insert( itr );
                remove_by_index(added_, itr);
            }

            bool is_removed( size_t itr )
            {
                guard_type lck(tmp_lock_);
                return (removed_.erase( itr ) != 0);
            }

            static
            void remove_by_index( list_type &lst, size_t id )
            {
                if( lst.size( ) > 0 ) {

                    size_t min_id = lst.begin( )->id_;
                    size_t max_id = lst.rbegin( )->id_;

                    if( (id < min_id) || (id > max_id) ) {

                        return;

                    } else if( (id - min_id) < (max_id - id) ) {
                        /// CLOSE to begin!
                        list_iterator b(lst.begin( ));

                        for( ; b && (b->id_<id); ++b );

                        if( b && ( b->id_ == id ) ) {
                            itr_erase( lst, b );
                        }

                    } else {
                        /// CLOSE to end!
                        list_iterator b(lst.rbegin( ));

                        for( ; b && (id < b->id_); --b );

                        if( b && (b->id_ == id) ) {
                            itr_rerase( lst, b );
                        }
                    }
                }
            }

            void clear_removed( )
            {
                iterator_set tmp;
                {
                    guard_type lck(tmp_lock_);
                    tmp.swap( removed_ );
                }

                typename iterator_set::iterator b(tmp.begin( ));
                typename iterator_set::iterator e(tmp.end( ));

                list_iterator bl(list_.begin( ));

                for( ; (b!=e) && bl; ++b ) {
                    for( ; bl && (bl->id_ < *b); ++bl );
                    if( bl && (bl->id_ == *b) ) {
                        bl = itr_erase( list_, bl );
                    }
                }
            }

            void clear_unsafe( ) /// unsafe
            {
                list_iterator b = list_.begin( );
                while( b ) {
                    b = itr_erase( list_, b );
                }

                b = added_.begin( );
                while( b ) {
                    b = itr_erase( added_, b );
                }

                removed_.clear( );
            }

            void clear( )
            {
                guard_type l0(list_lock_);
                guard_type l1(tmp_lock_);
                clear_unsafe( );
            }

            void splice_added( )
            {
                guard_type lck(tmp_lock_);
                list_.splice_back( added_ );
            }

            size_t connect( slot_type call )
            {
                guard_type l(tmp_lock_);
                size_t next = id_++;
                added_.push_back( slot_info(call, next) );
                return next;
            }

        };

        typedef vtrc::shared_ptr<impl>   impl_sptr;
        typedef vtrc::weak_ptr<impl>     impl_wptr;

        struct unsubscriber: subscription::unsubscriber {

            typedef base<SlotType, MutexType> base_type;

            typedef typename base_type::impl_sptr impl_sptr;
            typedef typename base_type::impl_wptr impl_wptr;

            unsubscriber( impl_wptr p, size_t k )
                :parent(p)
                ,key(k)
            { }

            vtrc::uintptr_t data( )
            {
                return reinterpret_cast<vtrc::uintptr_t>(parent.lock( ).get( ));
            }

            void run( )
            {
                impl_sptr lock(parent.lock( ));
                if( lock ) {
                    lock->unsubscribe( key );
                }
            }

            impl_wptr parent;
            size_t    key;
        };

    public:

        typedef common::observer::subscription subscription;
        typedef common::observer::scoped_subscription scoped_subscription;

        /// boost::signal compatibility
        typedef common::observer::subscription connection;
        typedef common::observer::scoped_subscription scoped_connection;

        friend class common::observer::subscription;
        friend class common::observer::scoped_subscription;

        base( )
            :impl_(vtrc::make_shared<impl>( ))
        { }

#if !VTRC_DISABLE_CXX11
        base( const base & )              = delete;
        base& operator = ( const base & ) = delete;
        base( base &&o )
        {
            impl_.swap( o.impl_ );
            o.impl_ = vtrc::make_shared<impl>( );
        }

        base & operator = ( base &&o )
        {
            impl_.swap( o.impl_ );
            o.impl_ = vtrc::make_shared<impl>( );
            return *this;
        }

#else
    private:
        base( const base & );
        base& operator = ( const base & );
    public:

#endif
        virtual ~base( )
        { }

        subscription subscribe( slot_type call )
        {
            size_t next = impl_->connect( call );

            subscription::unsubscriber_sptr us =
                vtrc::make_shared<unsubscriber>(impl_wptr(impl_), next);

            return subscription( us );
        }

        /// boost::signal compatibility
        subscription connect( slot_type call )
        {
            return subscribe( call );
        }

        void unsubscribe( subscription &cc )
        {
            cc.unsubscribe( );
        }

        /// boost::signal compatibility
        void disconnect( subscription &cc )
        {
            unsubscribe( cc );
        }

        void unsubscribe_all(  )
        {
            impl_->clear( );
        }

        /// boost::signal compatibility
        void disconnect_all_slots(  )
        {
            unsubscribe_all( );
        }

#if !VTRC_DISABLE_CXX11
        template <typename ...Args>
        void operator ( ) ( const Args& ...args )
        {
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            typename impl::list_iterator b(impl_->list_.begin( ));
            while( b ) {
                if( impl_->is_removed( b->id_ ) ) {
                    b = impl::itr_erase( impl_->list_, b );
                } else {
                    slot_traits::exec( b->slot_, args... );
                    if( slot_traits::expired( b->slot_ ) ) {
                        b = impl::itr_erase( impl_->list_, b );
                    } else {
                        ++b;
                    }
                }
            }
            impl_->clear_removed( );
        }

#else

#define VTRC_OBSERVER_OPERATOR_PROLOGUE \
            guard_type l(impl_->list_lock_); \
            impl_->splice_added( ); \
            typename impl::list_iterator b(impl_->list_.begin( )); \
            while( b ) { \
                if( !impl_->is_removed( b->id_ ) ) { \
                    slot_traits::exec( b->slot_

#define VTRC_OBSERVER_OPERATOR_EPILOGUE \
                    ); \
                    if( slot_traits::expired( b->slot_ ) ) { \
                        b = impl::itr_erase( impl_->list_, b ); \
                    } else { \
                        ++b; \
                    } \
                 } else { \
                    b = impl::itr_erase( impl_->list_, b ); \
                 } \
            } \
            impl_->clear_removed( )

        void operator ( ) ( )
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P>
        void operator ( ) ( const P& p0 )
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE,
                    p0
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1>
        void operator ( ) ( const P0& p0, const P1& p1 )
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2 )
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3 )
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4 )
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5 )
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4, p5
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5,
                  typename P6
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5,
                            const P6& p6 )
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4, p5, p6
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5,
                  typename P6, typename P7
                  >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5,
                            const P6& p6, const P7& p7 )
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4, p5, p6, p7
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5,
                  typename P6, typename P7,
                  typename P8
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5,
                            const P6& p6, const P7& p7,
                            const P8& p8 )
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4, p5, p6, p7, p8
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5,
                  typename P6, typename P7,
                  typename P8, typename P9
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5,
                            const P6& p6, const P7& p7,
                            const P8& p8, const P9& p9)
        {
            VTRC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4, p5, p6, p7, p8, p9
            VTRC_OBSERVER_OPERATOR_EPILOGUE;
        }

#undef VTRC_OBSERVER_OPERATOR_PROLOGUE
#undef VTRC_OBSERVER_OPERATOR_EPILOGUE

#endif
    private:
        impl_sptr impl_;
    };

}}}

#endif // SIMPLE_H

