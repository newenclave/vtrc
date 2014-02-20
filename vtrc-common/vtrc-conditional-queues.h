#ifndef VTRC_CONDITIONAL_QUEUES_H
#define VTRC_CONDITIONAL_QUEUES_H

#include <deque>
#include <boost/thread/condition_variable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

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

        enum wait_result {
             WAIT_RESULT_TIMEOUT    = 0
            ,WAIT_RESULT_SUCCESS    = 1
            ,WAIT_RESULT_CANCELED   = 2
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

        static bool queue_empty_predic( hold_value_type_sptr value )
        {
            return (!value->data_.empty( )) || value->canceled_;
        }

        static void pop_all( hold_value_type_sptr &value, queue_type &result )
        {
            queue_type tmp;
            tmp.swap( value->data_ );
            result.swap( tmp );
        }

        static wait_result cancel_res2wait_res ( bool canceled, bool waitres )
        {
            return canceled ? WAIT_RESULT_CANCELED
                            : ( waitres ? WAIT_RESULT_SUCCESS
                                        : WAIT_RESULT_TIMEOUT );
        }

        template <typename WaitFunc>
        wait_result wait_queue_impl( const key_type &key, WaitFunc call_wait )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(at(key));

            hold_value_type_sptr value( f->second );

            value->canceled_ = false;

            bool res = call_wait( lck, value );

            return cancel_res2wait_res( value->canceled_, res );
        }

        template <typename WaitFunc>
        wait_result read_queue_impl( const key_type &key,  queue_type &result,
                                     WaitFunc call_wait )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(at(key));

            hold_value_type_sptr value( f->second );

            value->canceled_ = false;

            bool res = call_wait( lck, value );

            if( res ) pop_all( value, result );

            return cancel_res2wait_res( value->canceled_, res );
        }

        template <typename WaitFunc>
        wait_result read_impl( const key_type &key,  queue_value_type &result,
                               WaitFunc call_wait )
        {
            unique_lock lck(lock_);
            typename map_type::iterator f(at(key));

            hold_value_type_sptr value( f->second );

            value->canceled_ = false;

            bool res = call_wait( lck, value );

            if( !value->canceled_ && !value->data_.empty( ) ) {
                std::swap( result, value->data_.front( ) );
                value->data_.pop_front( );
            }

            return cancel_res2wait_res( value->canceled_, res );
        }

        static bool cond_wait( unique_lock &lck,
                        hold_value_type_sptr &value )
        {
            value->cond_.wait( lck,
                          boost::bind( &this_type::queue_empty_predic, value ));
            return true;
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

        size_t queue_size( const key_type &key ) const
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

        void cancel_all( )
        {
            unique_lock lck(lock_);
            typedef typename map_type::iterator iterator_type;
            for( iterator_type b(store_.begin()), e(store_.end( )); b!=e; ++b) {
                b->second->canceled_ = true;
                b->second->cond_.notify_all( );
            }
        }

        void cancel( const key_type &key )
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

        wait_result wait_queue( const key_type &key )
        {
            return wait_queue_impl( key,
                        boost::bind( &this_type::cond_wait, _1, _2) );
        }

        wait_result read( const key_type &key, queue_value_type &result )
        {
            return read_impl( key, result,
                        boost::bind( &this_type::cond_wait, _1, _2 ) );
        }

        wait_result read_queue( const key_type &key, queue_type &result )
        {
            return read_queue_impl( key, result,
                        boost::bind( &this_type::cond_wait, _1, _2) );
        }

        bool queue_exists( const key_type &key ) const
        {
            unique_lock lck(lock_);
            return store_.find( key ) != store_.end( );
        }

#if defined BOOST_THREAD_USES_DATETIME

    private:

        template <typename TimeType>
        static bool cond_timed_wait( unique_lock &lck,
                        hold_value_type_sptr &value,
                        const TimeType &tt)
        {
            return value->cond_.timed_wait( lck, tt,
                          boost::bind( &this_type::queue_empty_predic, value ));
        }

    public:

        template <typename TimeType>
        wait_result wait_queue( const key_type &key, const TimeType &tt )
        {
            return wait_queue_impl( key,
                        boost::bind( &this_type::cond_timed_wait<TimeType>,
                                     _1, _2, boost::cref(tt) ) );
        }

        template <typename TimeType>
        wait_result read( const key_type &key, queue_value_type &result,
                                const TimeType &tt )
        {
            return read_impl( key, result,
                        boost::bind( &this_type::cond_timed_wait<TimeType>,
                                     _1, _2, boost::cref(tt) ) );
        }

        template <typename TimeType>
        wait_result read_queue( const key_type &key, queue_type &result,
                                const TimeType &tt )
        {
            return read_queue_impl( key, result,
                        boost::bind( &this_type::cond_timed_wait<TimeType>,
                                     _1, _2, boost::cref(tt) ) );
        }

#endif

#if defined BOOST_THREAD_USES_CHRONO

    private:

        template <typename Rep, typename Period>
        static bool cond_wait_for( unique_lock &lck,
                        hold_value_type_sptr &value,
                        const boost::chrono::duration<Rep, Period>& duration)
        {
            return value->cond_.wait_for( lck, duration,
                        boost::bind( &this_type::queue_empty_predic, value ));
        }

    public:
        template <typename Rep, typename Period>
        wait_result wait_queue( const key_type &key,
                         const boost::chrono::duration<Rep, Period>& duration )
        {
            return wait_queue_impl( key,
                        boost::bind( &this_type::cond_wait_for<Rep, Period>,
                                     _1, _2, boost::cref(duration) ) );
        }

        template <typename Rep, typename Period>
        wait_result read( const key_type &key, queue_value_type &result,
                         const boost::chrono::duration<Rep, Period>& duration )
        {
            return read_impl( key, result,
                        boost::bind( &this_type::cond_wait_for<Rep, Period>,
                                     _1, _2, boost::cref(duration) ) );
        }

        template <typename Rep, typename Period>
        wait_result read_queue( const key_type &key, queue_type &result,
                         const boost::chrono::duration<Rep, Period>& duration )
        {
            return read_queue_impl( key, result,
                        boost::bind( &this_type::cond_wait_for<Rep, Period>,
                                     _1, _2, boost::cref(duration) ) );
        }
#endif

    };

}}

#endif // VTRC_CONDITIONAL_QUEUES_H

