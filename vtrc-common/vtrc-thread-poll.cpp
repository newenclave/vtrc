#include "vtrc-thread-poll.h"

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <list>

namespace vtrc { namespace common {

    namespace basio = boost::asio;

    struct thread_poll::thread_poll_impl {

        struct thread_context {

            typedef boost::shared_ptr<thread_context> shared_type;
            boost::thread *thread_;
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

        std::list<thread_context::shared_type> threads_;
        mutable boost::shared_mutex            threads_lock_;

        struct interrupt {
            static void raise ( ) { throw interrupt( ); }
        };

        thread_poll_impl( basio::io_service *ios, bool own_ios )
            :ios_(ios)
            ,wrk_(new basio::io_service::work(*ios_))
            ,own_ios_(own_ios)
        {}

        ~thread_poll_impl( )
        {
            try {
                stop( );
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
            boost::lock_guard<boost::shared_mutex> lck(threads_lock_);
            threads_.insert(threads_.end(), tmp.begin(), tmp.end());
        }

        void inc( )
        {
            thread_context::shared_type n(create_new_context( ));
            boost::lock_guard<boost::shared_mutex> lck(threads_lock_);
            threads_.push_back(n);
        }

        thread_context::shared_type create_new_context( )
        {
            thread_context::shared_type
                    new_context(boost::make_shared<thread_context>());
            new_context->thread_ = new boost::thread(
                        boost::bind(&thread_poll_impl::run, this,
                                                    new_context.get( )) );
            return new_context;
        }

        void interrupt_one( )
        {
            ios_->post( interrupt::raise );
        }

        size_t size( ) const
        {
            boost::shared_lock<boost::shared_mutex> lck(threads_lock_);
            return threads_.size( );
        }

        void join_all( )
        {
            typedef std::list<thread_context::shared_type>::iterator iterator;
            boost::shared_lock<boost::shared_mutex> lck(threads_lock_);
            for(iterator b(threads_.begin()), e(threads_.end()); b!=e; ++b ) {
                if( (*b)->thread_->joinable( ) )
                    (*b)->thread_->join( );
            }
        }

        void stop( )
        {
            ios_->stop( );
            join_all( );
        }

        void run( thread_context *context )
        {
            run_impl( context );
            context->in_use_ = false;
        }

        void run_impl( thread_context * /*context*/ )
        {
            while ( true  ) {
                try {
                    while ( true ) {
                        const size_t count = ios_->run_one();
                        if( !count ) return;
                    }
                } catch( const interrupt & ) {
                    break;
                } catch( const boost::thread_interrupted & ) {
                    break;
                } catch( const std::exception &) {
                    ;;;
                }
            }
        }

    };

    thread_poll::thread_poll( )
        :impl_(new thread_poll_impl(new basio::io_service, true))
    {}

    thread_poll::thread_poll( size_t init_count )
        :impl_(new thread_poll_impl(new basio::io_service, true))
    {
        impl_->add( init_count );
    }

    thread_poll::thread_poll( boost::asio::io_service &ios, size_t init_count )
        :impl_(new thread_poll_impl(&ios, false))
    {
        impl_->add( init_count );
    }

    thread_poll::thread_poll( boost::asio::io_service &ios )
        :impl_(new thread_poll_impl(&ios, false))
    {}

    thread_poll::~thread_poll( )
    {
        delete impl_;
    }

    size_t thread_poll::size( ) const
    {
        return impl_->size( );
    }

    void thread_poll::interrupt_one( )
    {
        impl_->interrupt_one( );
    }

    void thread_poll::stop( )
    {
        impl_->stop( );
    }

    void thread_poll::join_all( )
    {
        impl_->join_all( );
    }

    void thread_poll::add_thread( )
    {
        impl_->inc( );
    }

    void thread_poll::add_threads( size_t count )
    {
        impl_->add(count);
    }

    boost::asio::io_service &thread_poll::get_io_service( )
    {
        return *impl_->ios_;
    }

    const boost::asio::io_service &thread_poll::get_io_service( ) const
    {
        return *impl_->ios_;
    }
}}
