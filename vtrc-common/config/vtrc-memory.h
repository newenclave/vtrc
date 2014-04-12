#ifndef VTRC_MEMORY_H
#define VTRC_MEMORY_H

#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/enable_shared_from_this.hpp"

namespace vtrc {

    using boost::shared_ptr;
    using boost::weak_ptr;
    using boost::make_shared;
    using boost::enable_shared_from_this;

/// will be add to config.h soon
#ifndef STD_HAS_UNIQUE_PTR_IMPL

    template<class T>
    class unique_ptr // noncopyable
    {
        T *ptr_;

        unique_ptr( unique_ptr const & );
        unique_ptr &operator = ( unique_ptr const & );

        typedef unique_ptr<T> this_type;

        void operator == ( unique_ptr const& ) const;
        void operator != ( unique_ptr const& ) const;

    public:

        typedef T element_type;

        explicit unique_ptr( element_type *p = NULL ) // throws( )
            :ptr_( p )
        { }

        ~unique_ptr( ) // throws( )
        {
            typedef char complete_type[ sizeof(element_type) ? 1 : -1 ];
            (void) sizeof(complete_type);
            delete ptr_;
        }

        void reset( element_type *p = NULL ) // throws( )
        {
            this_type( p ).swap( *this );
        }

        element_type &operator * ( ) const // throws( )
        {
            return *ptr_;
        }

        element_type *operator -> ( ) const
        {
            assert( ptr_ != 0 );
            return ptr_;
        }

        element_type *get( ) const
        {
            return ptr_;
        }

        void swap(unique_ptr & other) // throws( )
        {
            element_type *
            tmp        = other.ptr_;
            other.ptr_ = ptr_;
            ptr_       = tmp;
        }

    };

#endif

//    template <typename T>
//    struct shared_ptr { typedef boost::shared_ptr<T> type; };

//    template <typename T>
//    struct weak_ptr { typedef boost::weak_ptr<T> type; };
}

#endif // VTRCMEMORY_H
