#include <list>
#include <set>
#include <iostream>

#include "vtrc-asio.h"

#include "vtrc-mutex.h"
#include "vtrc-memory.h"
#include "vtrc-bind.h"
#include "vtrc-thread.h"

#include "vtrc/common/thread-pool.h"

#if VTRC_DISABLE_CXX11

#include "boost/thread/tss.hpp"

#endif

///// TODO: need some fix here
namespace vtrc { namespace common {

    namespace basio = VTRC_ASIO;

    typedef vtrc::unique_ptr<vtrc::thread> thread_ptr;

    struct thread_context {
        typedef vtrc::shared_ptr<thread_context> shared_type;
        thread_ptr thread_;
    };

    inline bool operator < ( const thread_context &l, const thread_context &r )
    {
        return  l.thread_.get( ) < r.thread_.get( );
    }

    static void decorator_default( thread_pool::call_decorator_type )
    { }

    static void exception_default( )
    {
        try {
            throw;
        } catch( const std::exception &ex ) {
            std::cerr << "Exception in thread 0x" << std::hex
                      << vtrc::this_thread::get_id( )
                      << ". what = " << ex.what( )
                         ;
        } catch( ... ) {
            std::cerr << "Exception '...' in thread 0x" << std::hex
                      << vtrc::this_thread::get_id( )
                         ;
        }
    }

    typedef std::set<thread_context::shared_type>  thread_set_type;

#if 0
    struct thread_pool::impl_ {
        basio::io_service              *ios_;
        basio::io_service::work        *wrk_;
        bool                            own_ios_;
        thread_pool::exception_handler  exception_;
        thread_set_type                 threads_;

        impl( basio::io_service *ios, bool own_ios )
            :ios_(ios)
            ,wrk_(new basio::io_service::work(*ios_))
            ,own_ios_(own_ios)
            ,exception_(&exception_default)
        { }

        ~impl( )
        {
            try {
                if( wrk_ ) {
                    stop( );
                }
                join_all( );
            } catch( ... ) {
                ;;;
            }

            if( own_ios_ ) {
                delete ios_;
            }
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

        thread_context::shared_type create_thread(  )
        {

            thread_context::shared_type new_inst(
                        vtrc::make_shared<thread_context>( ));

            new_inst->thread_.reset(
                     new vtrc::thread(vtrc::bind(&impl::run, this, new_inst)));

            return new_inst;
        }

    };
#endif

    struct thread_pool::impl {

        typedef vtrc::lock_guard<vtrc::mutex> locker_type;

        basio::io_service       *ios_;
        basio::io_service::work *wrk_;
        bool                     own_ios_;

        thread_set_type          threads_;
        thread_set_type          stopped_threads_;

        mutable vtrc::mutex      threads_lock_;

        thread_pool::exception_handler exception_;
        thread_pool::thread_decorator  decorator_;

//        struct interrupt {
//            static void raise ( ) { throw interrupt( ); }
//        };

        impl( basio::io_service *ios, bool own_ios )
            :ios_(ios)
            ,wrk_(new basio::io_service::work(*ios_))
            ,own_ios_(own_ios)
            ,exception_(&exception_default)
            ,decorator_(&decorator_default)
        { }

        ~impl( )
        {
            try {
                if( wrk_ ) {
                    stop( );
                }
                join_all( );
            } catch( ... ) {
                ;;;
            }

            if( own_ios_ ) {
                delete ios_;
            }
        }

        void add( size_t count )
        {
            std::list<thread_context::shared_type> tmp;
            while( count-- ) {
                tmp.push_back(create_thread( ));
            }
            locker_type lck(threads_lock_);
            threads_.insert( tmp.begin( ), tmp.end( ) );
            stopped_threads_.clear( );
        }

        void inc( )
        {
            thread_context::shared_type n(create_thread( ));
            locker_type lck( threads_lock_ );
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

//        void interrupt_one( )
//        {
//            ios_->post( interrupt::raise );
//        }

        size_t size( ) const
        {
            locker_type lck(threads_lock_);
            return threads_.size( );
        }

        static bool is_this_id( const  thread_ptr &th )
        {
            return vtrc::this_thread::get_id( ) == th->get_id( );
        }

        void join_all( )
        {
            typedef thread_set_type::iterator iterator;
            thread_set_type tmp;
            {
                /// own threads
                locker_type lck(threads_lock_);
                tmp.swap( threads_ );
            }

            /// join
            for(iterator b(tmp.begin( )), e(tmp.end( )); b!=e; ++b ) {
                if( (*b)->thread_->joinable( ) && !is_this_id( (*b)->thread_) )
                    (*b)->thread_->join( );
            }

            /// clean stopped
            locker_type lck(threads_lock_);
            stopped_threads_.clear( );
        }

        void stop( )
        {

            delete wrk_;
            wrk_ = VTRC_NULL;

            ios_->stop( );
            locker_type lck(threads_lock_);
            stopped_threads_.clear( );
        }

        void move_to_stopped( thread_context::shared_type &thread )
        {
            locker_type lck(threads_lock_);

            stopped_threads_.insert( thread );
            threads_.erase( thread );
        }

        void run( thread_context::shared_type thread )
        {
            run_impl( thread.get( ), decorator_ );
//            move_to_stopped( thread ); /// must not remove it here;
//                                       /// moving to stopped_thread_
        }

        bool run_impl( thread_context * /*context*/,
                       thread_pool::thread_decorator td )
        {
            struct decoratot_keeper {
                thread_pool::thread_decorator td_;

                decoratot_keeper( thread_pool::thread_decorator td )
                    :td_(td)
                {
                    td_( thread_pool::CALL_PROLOGUE );
                }

                ~decoratot_keeper( )
                {
                    try {
                        td_( thread_pool::CALL_EPILOGUE );
                    } catch ( ... ) {

                    }
                }
            } _(td);

            while ( true  ) {
                try {
                    while ( true ) {
                        const size_t count = ios_->run_one( );
                        if( 0 == count ) {
                            return true; /// stopped;
                        }
                    }
//                } catch( const interrupt & ) {
//                    break;
                } catch( ... ) {
                    exception_( );
                }
            }
            return false; /// interruped by interrupt::raise or
                          ///               boost::thread_interrupted
        }

        bool attach( thread_pool::thread_decorator td )
        {
            return run_impl( VTRC_NULL, td );
        }

    };

    thread_pool::thread_pool(  )
        :impl_(new impl(new basio::io_service, true))
    { }

    thread_pool::thread_pool( size_t init_count )
        :impl_(new impl(new basio::io_service, true))
    {
        impl_->add( init_count );
    }

    thread_pool::thread_pool( VTRC_ASIO::io_service &ios, size_t init_count )
        :impl_(new impl(&ios, false))
    {
        impl_->add( init_count );
    }

    thread_pool::thread_pool( VTRC_ASIO::io_service &ios )
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

//    void thread_pool::interrupt_one( )
//    {
//        impl_->interrupt_one( );
//    }

    void thread_pool::stop( )
    {
        impl_->stop( );
    }

    void thread_pool::join_all( )
    {
        impl_->join_all( );
    }

    void thread_pool::assign_exception_handler( exception_handler eh )
    {
        impl_->exception_ = eh ? eh : exception_default;
    }

    void thread_pool::assign_thread_decorator( thread_pool::thread_decorator d )
    {
        impl_->decorator_ = d ? d : decorator_default;
    }


    bool thread_pool::attach(  )
    {
        return impl_->attach( impl_->decorator_ );
    }

    bool thread_pool::attach( thread_decorator td )
    {
        return impl_->attach( td );
    }

    void thread_pool::add_thread( )
    {
        impl_->inc( );
    }

    void thread_pool::add_threads( size_t count )
    {
        impl_->add(count);
    }

    VTRC_ASIO::io_service &thread_pool::get_io_service( )
    {
        return *impl_->ios_;
    }

    const VTRC_ASIO::io_service &thread_pool::get_io_service( ) const
    {
        return *impl_->ios_;
    }
}}
