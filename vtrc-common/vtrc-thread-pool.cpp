#include <list>

#include "boost/asio.hpp"

#include "vtrc-thread-pool.h"
#include "vtrc-mutex-typedefs.h"
#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-thread.h"
#include "vtrc-atomic.h"

namespace vtrc { namespace common {

    namespace basio = boost::asio;

    struct thread_pool::impl {

        struct thread_context {

            typedef vtrc::shared_ptr<thread_context> shared_type;
            vtrc::thread  *thread_;
            bool           in_use_;

            thread_context()
                :thread_(NULL)
                ,in_use_(true)
            { }

            ~thread_context()
            {
                if( thread_ )
                    delete thread_;
            }
        };

        basio::io_service       *ios_;
        basio::io_service::work *wrk_;
        bool                     own_ios_;
        //vtrc::atomic<size_t>     count_;

        std::list<thread_context::shared_type>  threads_;
        mutable shared_mutex                    threads_lock_;

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

            delete wrk_;

            if( own_ios_ ) {
                delete ios_;
            }
        }

        void add( size_t count )
        {
            std::list<thread_context::shared_type> tmp;
            while( count-- ) {
                tmp.push_back(create_new_context( ));
            }
            unique_shared_lock lck(threads_lock_);
            threads_.insert(threads_.end(), tmp.begin(), tmp.end());
        }

        void inc( )
        {
            thread_context::shared_type n(create_new_context( ));
            unique_shared_lock lck(threads_lock_);
            threads_.push_back(n);
        }

        thread_context::shared_type create_new_context( )
        {
            thread_context::shared_type
                    new_context(vtrc::make_shared<thread_context>());
            new_context->thread_ = new vtrc::thread(
                        vtrc::bind(&impl::run, this, new_context.get( )) );
            return new_context;
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
            typedef std::list<thread_context::shared_type>::iterator iterator;
            shared_lock lck(threads_lock_);
            for(iterator b(threads_.begin()), e(threads_.end()); b!=e; ++b ) {
                if( (*b)->thread_->joinable( ) )
                    (*b)->thread_->join( );
            }
        }

        void stop( )
        {
            ios_->stop( );
        }

        void run( thread_context *context )
        {
            run_impl( context );
            context->in_use_ = false;
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
