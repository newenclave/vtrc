#ifndef VTRC_CONDITION_QUEUES_H
#define VTRC_CONDITION_QUEUES_H

#include <deque>
#include "vtrc-condition-variable.h"
#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-mutex.h"
#include "vtrc-ref.h"
#include "vtrc-chrono.h"

#include "vtrc-result-codes.h"
#include "vtrc-mutex-typedefs.h"

namespace vtrc { namespace common {

    template <typename KeyType, typename QueueValueType>
    class condition_queues {

        typedef condition_queues                this_type;
        typedef vtrc::shared_mutex              mutex_type;

        typedef vtrc::unique_lock<vtrc::mutex>  unique_lock;

    public:

        typedef KeyType                         key_type;
        typedef QueueValueType                  queue_value_type;
        typedef std::deque<queue_value_type>    queue_type;

    private:

        typedef wait_result_codes wait_result;

        struct hold_value_type {

            mutable vtrc::mutex       lock_;
            vtrc::condition_variable  cond_;
            mutable bool              canceled_;
            queue_type                data_;

            hold_value_type( )
                :canceled_(false)
            { }
        };

        typedef vtrc::shared_ptr<hold_value_type> hold_value_type_sptr;
        typedef std::map<key_type, hold_value_type_sptr> map_type;

    private:

        map_type            store_;
        mutable mutex_type  lock_;

    private:

        typename map_type::iterator at( const key_type &key )
        {
            typename map_type::iterator f(store_.find( key ));
            if( f == store_.end( ) ) {
                throw std::out_of_range( "Bad queue index" );
            }
            return f;
        }

        hold_value_type_sptr value_at_key( const key_type &key )
        {
            return at( key )->second;
        }

        hold_value_type_sptr value_by_key( const key_type &key )
        {
            typename map_type::iterator f(store_.find( key ));
            return (f == store_.end( )) ? hold_value_type_sptr( )
                                        : f->second;
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
                                        :  WAIT_RESULT_TIMEOUT );
        }

        template <typename WaitFunc>
        wait_result wait_queue_impl( const key_type &key, WaitFunc call_wait )
        {

            hold_value_type_sptr value;
            {
                vtrc::shared_lock lck(lock_);
                value = value_at_key( key );
            }

            unique_lock lck(value->lock_);

            value->canceled_ = false;
            bool res = call_wait( lck, value );

            return cancel_res2wait_res( value->canceled_, res );
        }

        template <typename WaitFunc>
        wait_result read_queue_impl( const key_type &key,  queue_type &result,
                                     WaitFunc call_wait )
        {

            hold_value_type_sptr value;
            {
                vtrc::shared_lock lck(lock_);
                value = value_at_key( key );
            }

            unique_lock lck(value->lock_);
            value->canceled_ = false;
#if 1
            bool res = true;
            if( value->data_.empty( ) ) {
                res = call_wait( lck, value );
            }
#else
            bool res = call_wait( lck, value );
#endif
            
            if( res ) pop_all( value, result );

            return cancel_res2wait_res( value->canceled_, res );
        }

        template <typename WaitFunc>
        wait_result read_impl( const key_type &key, queue_value_type &result,
                               WaitFunc call_wait )
        {

            hold_value_type_sptr value;
            {
                vtrc::shared_lock lck(lock_);
                value = value_at_key( key );
            }

            unique_lock lck(value->lock_);
            value->canceled_ = false;
#if 1
            bool res = true;
            if( value->data_.empty( ) ) {
                res = call_wait( lck, value );
            }
#else
            bool res = call_wait( lck, value );
#endif
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
                          vtrc::bind( &this_type::queue_empty_predic, value ));
            return true;
        }

        condition_queues( const condition_queues &other );
        condition_queues& operator = (const condition_queues &other);

    public:

        condition_queues( )
        { }

        ~condition_queues( ) try
        {
            cancel_all( );
        } catch ( ... ) { ;;; }

        size_t size( ) const
        {
            vtrc::shared_lock lck(lock_);
            return store_.size( );
        }

        size_t queue_size( const key_type &key ) const
        {
            vtrc::shared_lock lck(lock_);
            return at(key)->second->data_.size( );
        }

        void add_queue( const key_type &key )
        {
            vtrc::upgradable_lock lck(lock_);
            typename map_type::iterator f(store_.find( key ));
            if( f == store_.end( ) ) {
                vtrc::upgrade_to_unique ulck(lck);
                store_.insert( std::make_pair( key,
                                    vtrc::make_shared<hold_value_type>( ) ));
            }
        }

        void erase_queue( const key_type &key )
        {
            vtrc::upgradable_lock lck(lock_);
            typename map_type::iterator f(store_.find( key ));
            if( f != store_.end( ) ) {
                vtrc::upgrade_to_unique ulck(lck);
                f->second->canceled_ = true;
                f->second->cond_.notify_all( );
                store_.erase( f );
            }
        }

        void erase_all(  )
        {
            vtrc::unique_shared_lock lck(lock_);
            for(typename map_type::iterator b(store_.begin()), e(store_.end());
                                            b!=e; ++b)
            {
                b->second->canceled_ = true;
                b->second->cond_.notify_all( );
            }
            store_.clear( );
        }

        void cancel_all( )
        {
            vtrc::shared_lock lck(lock_);
            typedef typename map_type::iterator iterator_type;
            for( iterator_type b(store_.begin( )), e(store_.end( )); b!=e; ++b){
                b->second->canceled_ = true;
                b->second->cond_.notify_all( );
            }
        }

        void cancel( const key_type &key )
        {
            vtrc::shared_lock lck(lock_);
            typename map_type::iterator f(at( key ));
            f->second->canceled_ = true;
            f->second->cond_.notify_all( );
        }

        void write_queue( const key_type &key, const queue_value_type &data )
        {
            vtrc::shared_lock lck(lock_);
            typename map_type::iterator f(at( key ));

            f->second->data_.push_back( data );
            f->second->cond_.notify_one( );
        }

        static void write_all_impl( typename map_type::value_type &f,
                                    const queue_value_type &data )
        {

            f.second->data_.push_back( data );
            f.second->cond_.notify_one( );
        }

        void write_all( const queue_value_type &data )
        {
            vtrc::shared_lock lck(lock_);
            std::for_each( store_.begin( ), store_.end( ),
                           vtrc::bind( this_type::write_all_impl,
                                       _1, vtrc::cref(data)));
        }

        void write_queue_if_exists( const key_type &key,
                                    const queue_value_type &data)
        {

            hold_value_type_sptr value;
            {
                vtrc::shared_lock lck(lock_);
                value = value_by_key( key );
            }

            if( value )  {
                vtrc::unique_lock lck(value->lock_);
                value->data_.push_back( data );
                value->cond_.notify_one( );
            }
        }

        void write_queue( const key_type &key, const queue_type &data )
        {
            vtrc::shared_lock lck(lock_);
            typename map_type::iterator f(at( key ));

            vtrc::unique_lock lck(f->second->lock_);

            f->second->data_.insert( f->second->data_.end( ),
                                     data.begin( ), data.end( ));
            f->second->cond_.notify_one( );
        }

        wait_result_codes wait_queue( const key_type &key )
        {
            return wait_queue_impl( key,
                        vtrc::bind( &this_type::cond_wait, _1, _2) );
        }

        wait_result_codes read( const key_type &key, queue_value_type &result )
        {
            return read_impl( key, result,
                        vtrc::bind( &this_type::cond_wait, _1, _2 ) );
        }

        wait_result_codes read_queue( const key_type &key, queue_type &result )
        {
            return read_queue_impl( key, result,
                        vtrc::bind( &this_type::cond_wait, _1, _2) );
        }

        bool queue_exists( const key_type &key ) const
        {
            unique_lock lck(lock_);
            return store_.find( key ) != store_.end( );
        }

#if 0 // defined BOOST_THREAD_USES_DATETIME

    private:

        template <typename TimeType>
        static bool cond_timed_wait( unique_lock &lck,
                        hold_value_type_sptr &value,
                        const TimeType &tt)
        {
            return value->cond_.timed_wait( lck, tt,
                          vtrc::bind( &this_type::queue_empty_predic, value ));
        }

    public:

        template <typename TimeType>
        queue_wait_result wait_queue( const key_type &key, const TimeType &tt )
        {
            return wait_queue_impl( key,
                        vtrc::bind( &this_type::cond_timed_wait<TimeType>,
                                     _1, _2, vtrc::cref(tt) ) );
        }

        template <typename TimeType>
        queue_wait_result read( const key_type &key, queue_value_type &result,
                                const TimeType &tt )
        {
            return read_impl( key, result,
                        vtrc::bind( &this_type::cond_timed_wait<TimeType>,
                                     _1, _2, vtrc::cref(tt) ) );
        }

        template <typename TimeType>
        queue_wait_result read_queue( const key_type &key, queue_type &result,
                                const TimeType &tt )
        {
            return read_queue_impl( key, result,
                        vtrc::bind( &this_type::cond_timed_wait<TimeType>,
                                     _1, _2, vtrc::cref(tt) ) );
        }

#endif

    private:

        template <typename Rep, typename Period>
        static bool cond_wait_for( unique_lock &lck,
                        hold_value_type_sptr &value,
                        const vtrc::chrono::duration<Rep, Period>& duration)
        {
            return value->cond_.wait_for( lck, duration,
                        vtrc::bind( &this_type::queue_empty_predic, value ));
        }

    public:
        template <typename Rep, typename Period>
        wait_result_codes wait_queue( const key_type &key,
                         const vtrc::chrono::duration<Rep, Period>& duration )
        {
            return wait_queue_impl( key,
                        vtrc::bind( &this_type::cond_wait_for<Rep, Period>,
                                     _1, _2, vtrc::cref(duration) ) );
        }

        template <typename Rep, typename Period>
        wait_result_codes read( const key_type &key, queue_value_type &result,
                         const vtrc::chrono::duration<Rep, Period>& duration )
        {
            return read_impl( key, result,
                        vtrc::bind( &this_type::cond_wait_for<Rep, Period>,
                                     _1, _2, vtrc::cref(duration) ) );
        }

        template <typename Rep, typename Period>
        wait_result_codes read_queue( const key_type &key, queue_type &result,
                         const vtrc::chrono::duration<Rep, Period>& duration )
        {
            return read_queue_impl( key, result,
                        vtrc::bind( &this_type::cond_wait_for<Rep, Period>,
                                     _1, _2, vtrc::cref(duration) ) );
        }

    };

}}

#endif // VTRC_CONDITIONAL_QUEUES_H

