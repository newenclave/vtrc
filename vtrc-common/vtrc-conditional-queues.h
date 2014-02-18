#ifndef VTRC_CONDITIONAL_QUEUES_H
#define VTRC_CONDITIONAL_QUEUES_H

#include <deque>
#include <boost/thread/condition_variable.hpp>

namespace vtrc { namespace common {

    template <typename KeyType, typename QueueValueType>
    class conditional_queues {

        typedef conditional_queues              this_type;
        typedef boost::mutex                    mutex_type;
        typedef boost::unique_lock<mutex_type>  unique_lock;

    public:

        typedef KeyType                         key_type;
        typedef QueueValueType                  queue_value_type;
        typedef std::deque<queue_value_type>    queue_type;

        enum read_result {
             READ_TIMEOUT  = 0
            ,READ_SUCCESS
            ,READ_CANCELED
        };

    private:

        struct hold_value_type {
            boost::condition_variable cond_;
            queue_type                data_;
            bool                      canceled_;
            hold_value_type( )
                :canceled_(false)
            {}
        };

        typedef boost::shared_ptr<hold_value_type> hold_value_type_sptr;
        typedef std::map<key_type, hold_value_type_sptr> map_type;

    private:

        map_type            store_;
        mutable mutex_type  lock_;

    private:

        typename map_type::iterator at( const key_type &key )
        {
            typename map_type::iterator f(store_.find( key ));
            if( f == store_.end( ) ) {
                throw std::out_of_range( "Bad index" );
            }
            return f;
        }

        typename map_type::const_iterator at( const key_type &key ) const
        {
            typename map_type::const_iterator f(store_.find( key ));
            if( f == store_.end( ) ) {
                throw std::out_of_range( "Bad index" );
            }
            return f;
        }

        static bool queue_empty( hold_value_type_sptr value )
        {
            return (!value->data_.empty( )) || value->canceled_;
        }

        conditional_queues( const conditional_queues &other );
        conditional_queues& operator = (const conditional_queues &other);

    public:

        conditional_queues( )
        { }

        ~conditional_queues( )
        {
            cancel_all( );
        }

        size_t size( ) const
        {
            unique_lock lck(lock_);
            return store_.size( );
        }

        size_t queue_size( const key_type &key )  const
        {
            unique_lock lck(lock_);
            typename map_type::const_iterator f(at(key));
            return f->second->data_.size( );
        }

        void add_queue( const key_type &key )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(store_.find( key ));
            if( f == store_.end( ) ) {
                store_.insert(
                    std::make_pair( key,
                        boost::make_shared<hold_value_type>( ) ));
            }
        }

        void erase_queue( const key_type &key )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(store_.find( key ));
            if( f != store_.end( ) ) {
                f->second->canceled_ = true;
                f->second->cond_.notify_all( );
                store_.erase( f );
            }
        }

        void cancel_all(  )
        {
            unique_lock lck(lock_);
            typedef typename map_type::iterator iterator_type;
            for( iterator_type b(store_.begin()), e(store_.end( )); b!=e; ++b) {
                b->second->canceled_ = true;
                b->second->cond_.notify_all( );
            }
        }

        void cancel_queue( const key_type &key )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(at( key ));
            f->second->canceled_ = true;
            f->second->cond_.notify_all( );
        }

        void write_queue( const key_type &key, const queue_value_type &data )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(at( key ));

            f->second->data_.push_back( data );
            f->second->cond_.notify_all( );
        }

        void write_queue( const key_type &key, const queue_type &data )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(at( key ));

            f->second->data_.insert( f->second->data_.end( ),
                                     data.begin( ), data.end( ));
            f->second->cond_.notify_all( );
        }

        read_result read( const key_type &key, queue_value_type &result )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(at( key ));
            hold_value_type_sptr value( f->second );

            value->canceled_ = false;

            value->cond_.wait( lck,
                boost::bind( &this_type::queue_empty, value ));

            if( !value->canceled_ && !value->data_.empty( ) ) {
                std::swap( result, value->data_.front( ) );
                value->data_.pop_front( );
            }

            return value->canceled_ ? READ_CANCELED : READ_SUCCESS;
        }

        read_result read_queue( const key_type &key, queue_type &result )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(at( key ));
            hold_value_type_sptr value( f->second );

            value->canceled_ = false;

            value->cond_.wait( lck,
                boost::bind( &this_type::queue_empty, value ));

            if( !value->canceled_ ) {
                queue_type tmp;
                tmp.swap( value->data_ );
                result.swap( tmp );
            }

            return value->canceled_ ? READ_CANCELED : READ_SUCCESS;
        }

#if defined BOOST_THREAD_USES_DATETIME
        template <typename TimeType>
        read_result read_queue( const key_type &key, queue_type &result,
                                const TimeType &period )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(at(key));

            hold_value_type_sptr value( f->second );

            value->canceled_ = false;

            bool res = value->cond_.timed_wait( lck, period,
                    boost::bind( &this_type::queue_empty, value ) );

            if( res ) {
                std::deque<std::string> tmp;
                tmp.swap( value->data_ );
                result.swap( tmp );
            }

            return value->canceled_
                ? READ_CANCELED
                : (res ? READ_SUCCESS : READ_TIMEOUT);
        }
#endif

#ifdef BOOST_THREAD_USES_CHRONO

        template <class Rep, class Period>
        read_result read_queue( const key_type &key, queue_type &result,
                         const boost::chrono::duration<Rep, Period>& duration )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(at(key));

            hold_value_type_sptr value( f->second );

            value->canceled_ = false;

            bool res = value->cond_.wait_for( lck, duration,
                   boost::bind( &this_type::queue_empty, value ));

            if( res ) {
                std::deque<std::string> tmp;
                tmp.swap( value->data_ );
                result.swap( tmp );
            }

            return value->canceled_
                ? READ_CANCELED : (res ? READ_SUCCESS : READ_TIMEOUT);
        }
#endif

        bool queue_exists( const key_type &key ) const
        {
            unique_lock lck(lock_);
            return store_.find( key ) != store_.end( );
        }

    };

}}

#endif // VTRCCONDITIONALQUEUES_H

