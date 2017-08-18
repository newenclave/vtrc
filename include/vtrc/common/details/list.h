#ifndef VTRC_COMMON_DETAILS_LIST_H
#define VTRC_COMMON_DETAILS_LIST_H

#include "vtrc/common/config/vtrc-stdint.h"
#include "vtrc/common/config/vtrc-cxx11.h"

namespace vtrc { namespace common { namespace details {

    //// NOT STL LIST!!!
    template <typename T>
    class list {

        struct node {
            node *prev_;
            node *next_;
            T data_;
            node( const T &data )
                :prev_(VTRC_NULL)
                ,next_(VTRC_NULL)
                ,data_(data)
            { }

            T &get( )
            {
                return data_;
            }

            const T &get( ) const
            {
                return data_;
            }

            T * operator -> ( )
            {
                return &data_;
            }

            const T * operator -> ( ) const
            {
                return &data_;
            }
        };

    public:

        typedef T value_type;

        class iterator {

            friend class list<T>;
            node *node_;
            iterator(node *n)
                :node_(n)
            { }

        public:

            vtrc::uintptr_t ptr( ) const
            {
                return reinterpret_cast<vtrc::uintptr_t>(node_);
            }

            T & operator * ( )
            {
                return node_->get( );
            }

            const T & operator * ( ) const
            {
                return node_->get( );
            }

            T * operator -> ( )
            {
                return &node_->data_;
            }

            const T * operator -> ( ) const
            {
                return &node_->data_;
            }

            /// prefix
            iterator operator ++ ( )
            {
                node_ = node_->next_;
                return iterator(node_);
            }

            /// prefix
            iterator operator -- ( )
            {
                node_ = node_->prev_;
                return iterator(node_);
            }

            /// postfix
            iterator operator ++ ( int )
            {
                iterator tmp(node_);
                node_ = node_->next_;
                return tmp;
            }

            /// postfix
            iterator operator -- ( int )
            {
                iterator tmp(node_);
                node_ = node_->prev_;
                return tmp;
            }

            bool operator == ( const iterator &o ) const
            {
                return node_ == o.node_;
            }

            bool operator != ( const iterator &o ) const
            {
                return node_ != o.node_;
            }

            operator bool ( ) const
            {
                return node_ != VTRC_NULL;
            }
        };

    private:

        void cerase( iterator itr )
        {
            if( itr.node_->prev_ ) {
                itr.node_->prev_->next_ = itr.node_->next_;
            }

            if( itr.node_->next_ ) {
                itr.node_->next_->prev_ = itr.node_->prev_;
            }

            if( itr.node_ == back_ ) {
                back_ = itr.node_->prev_;
            }

            if( itr.node_ == front_ ) {
                front_ = itr.node_->next_;
            }

            delete itr.node_;
            size_--;

        }
    public:

        list( )
            :front_(VTRC_NULL)
            ,back_(VTRC_NULL)
            ,size_(0)
        { }

        list( const list &other )
            :front_(VTRC_NULL)
            ,back_(VTRC_NULL)
            ,size_(0)
        {
            list l(other.begin( ), other.end( ));
            swap( l );
        }

        template <typename Itr>
        list( Itr begin, Itr end )
            :front_(VTRC_NULL)
            ,back_(VTRC_NULL)
            ,size_(0)
        {
            list l;
            for( ; begin != end; ++begin ) {
                l.push_back( *begin );
            }
            swap( l );
        }

        ~list( )
        {
            clear( );
        }

        void clear( )
        {
            node *p = front_;
            while( p ) {
                node *tmp = p->next_;
                delete p;
                p = tmp;
            }
            front_ = back_ = VTRC_NULL;
        }

        iterator begin( )
        {
            return iterator(front_);
        }

        iterator rbegin( )
        {
            return iterator(back_);
        }

        iterator end( )
        {
            return iterator(VTRC_NULL);
        }

        iterator rend( )
        {
            return end( );
        }

        iterator rerase( iterator itr )
        {
            if( itr.node_ ) {
                node *tmp = itr.node_->prev_;
                cerase( itr );
                return iterator(tmp);
            } else {
                return itr;
            }

        }

        iterator erase( iterator itr )
        {
            if( itr.node_ ) {
                node *tmp = itr.node_->next_;
                cerase( itr );
                return iterator(tmp);
            } else {
                return itr;
            }
        }

        void splice_back( list<T> &other )
        {
            if( &other != this ) {
                if( front_ == VTRC_NULL ) {
                    front_ = other.front_;
                    back_  = other.back_;
                } else if( other.front_ ) {
                    back_->next_        = other.front_;
                    other.front_->prev_ = back_;
                    back_               = other.back_;
                }
                other.front_ = other.back_= VTRC_NULL;
                size_       += other.size( );
                other.size_  = 0;
            }
        }

        void push_back( const T &data )
        {
            node *new_node = new node(data);
            if( front_ ) {
                new_node->prev_ = back_;
                back_->next_    = new_node;
                back_           = new_node;
            } else {
                back_ = front_  = new_node;
            }
            size_++;
        }

        void push_front( const T &data )
        {
            node *new_node = new node(data);
            if( front_ ) {
                new_node->next_ = front_;
                front_->prev_   = new_node;
                front_          = new_node;
            } else {
                back_ = front_  = new_node;
            }
            size_++;
        }

        void swap( list<T> &other )
        {
            node *tf     = front_;
            front_       = other.front_;
            other.front_ = tf;

            node *tb     = back_;
            back_        = other.back_;
            other.back_  = tb;

            size_t ts    = size_;
            size_        = other.size_;
            other.size_  = ts;
        }

        size_t size( ) const
        {
            return size_;
        }

    private:

        node   *front_;
        node   *back_;
        size_t  size_;
    };


}}}

#endif // LIST_H
