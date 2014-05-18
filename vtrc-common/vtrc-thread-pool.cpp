#include <list>
#include <set>

#include "boost/asio.hpp"

#include "vtrc-thread-pool.h"
#include "vtrc-mutex-typedefs.h"
#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-thread.h"
#include "vtrc-atomic.h"

namespace vtrc { namespace common {

    namespace basio = boost::asio;

    typedef vtrc::shared_ptr<vtrc::thread> thread_sptr;

    struct thread_context {
        typedef vtrc::shared_ptr<thread_context> shared_type;
        thread_sptr thread_;
    };

    inline bool operator < ( const thread_context &l, const thread_context &r )
    {
        return  l.thread_.get( ) < r.thread_.get( );
    }

    typedef std::set<thread_context::shared_type>  thread_set_type;

    struct thread_pool::impl {

        basio::io_service       *ios_;
        basio::io_service::work *wrk_;
        bool                     own_ios_;

        thread_set_type          threads_;
        thread_set_type          stopped_threads_;

        mutable shared_mutex     threads_lock_;

        struct interrupt {
            static void raise ( ) { throw interrupt( ); }
        };

        impl( basio::io_service *ios, bool own_ios )
            :ios_(ios)
            ,wrk_(new basio::io_service::work(*ios_))
            ,own_ios_(own_ios)
        { }

        ~impl( )
        {
            try {
                stop( );
                join_all( );
            } catch( ... ) {
                ;;;
            }

            if( own_ios_ ) {
                delete ios_;
            }
        }

        void move_to_stopped( thread_context::shared_type &thread )
        {
            unique_shared_lock lck(threads_lock_);

            stopped_threads_.insert( thread );
            threads_.erase( thread );
        }

        void add( size_t count )
        {
            std::list<thread_context::shared_type> tmp;
            while( count-- ) {
                tmp.push_back(create_thread( ));
            }
            unique_shared_lock lck(threads_lock_);
            threads_.insert( tmp.begin( ), tmp.end( ) );
            stopped_threads_.clear( );
        }

        void inc( )
        {
            thread_context::shared_type n(create_thread( ));
            unique_shared_lock lck( threads_lock_ );
            threads_.insert( n );
            stopped_threads_.clear( );
        }

        thread_context::shared_type create_thread(  )
        {

            thread_context::shared_type new_inst(
                        vtrc::make_shared<thread_context>( ));

            new_inst->thread_.reset(
                     new vtrc::thread(vtrc::bind(&impl::run, this, new_inst)));

            return new_inst;
        }

        void interrupt_one( )
        {
            ios_->post( interrupt::raise );
        }

        size_t size( ) const
        {
            shared_lock lck(threads_lock_);
            return threads_.size( );
        }

        void join_all( )
        {
            typedef thread_set_type::iterator iterator;
            thread_set_type tmp;
            {
                /// own threads
                unique_shared_lock lck(threads_lock_);
                tmp.swap( threads_ );
            }

            /// join
            for(iterator b(tmp.begin( )), e(tmp.end( )); b!=e; ++b ) {
                if( (*b)->thread_->joinable( ) )
                    (*b)->thread_->join( );
            }

            /// clean stopped
            unique_shared_lock lck(threads_lock_);
            stopped_threads_.clear( );
        }

        void stop( )
        {

            delete wrk_;
            wrk_ = NULL;

            ios_->stop( );
            unique_shared_lock lck(threads_lock_);
            stopped_threads_.clear( );
        }

        void run( thread_context::shared_type thread )
        {
            run_impl( thread.get( ) );
            move_to_stopped( thread ); /// must not remove it here;
                                       /// moving to stopped_thread_
        }

        bool run_impl( thread_context * /*context*/ )
        {
            while ( true  ) {
                try {
                    while ( true ) {
                        const size_t count = ios_->run_one();
                        if( !count ) return true; /// stopped;
                    }
                } catch( const interrupt & ) {
                    break;
                } catch( const boost::thread_interrupted & ) {
                    break;
                } catch( const std::exception &) {
                    ;;;
                }
            }
            return false; /// interruped by interrupt::raise or
                          ///               boost::thread_interrupted
        }

        bool attach( )
        {
            return run_impl( NULL );
        }

    };

    thread_pool::thread_pool( )
        :impl_(new impl(new basio::io_service, true))
    { }

    thread_pool::thread_pool( size_t init_count )
        :impl_(new impl(new basio::io_service, true))
    {
        impl_->add( init_count );
    }

    thread_pool::thread_pool( boost::asio::io_service &ios, size_t init_count )
        :impl_(new impl(&ios, false))
    {
        impl_->add( init_count );
    }

    thread_pool::thread_pool( boost::asio::io_service &ios )
        :impl_(new impl(&ios, false))
    { }

    thread_pool::~thread_pool( )
    {
        delete impl_;
    }

    size_t thread_pool::size( ) const
    {
        return impl_->size( );
    }

    void thread_pool::interrupt_one( )
    {
        impl_->interrupt_one( );
    }

    void thread_pool::stop( )
    {
        impl_->stop( );
    }

    void thread_pool::join_all( )
    {
        impl_->join_all( );
    }

    bool thread_pool::attach( )
    {
        return impl_->attach( );
    }

    void thread_pool::add_thread( )
    {
        impl_->inc( );
    }

    void thread_pool::add_threads( size_t count )
    {
        impl_->add(count);
    }

    boost::asio::io_service &thread_pool::get_io_service( )
    {
        return *impl_->ios_;
    }

    const boost::asio::io_service &thread_pool::get_io_service( ) const
    {
        return *impl_->ios_;
    }
}}
