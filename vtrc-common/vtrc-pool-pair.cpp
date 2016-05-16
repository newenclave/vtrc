
#include "vtrc-pool-pair.h"
#include "vtrc-thread-pool.h"
#include "vtrc-memory.h"

namespace vtrc { namespace common {

    namespace basio = VTRCASIO;

    struct pool_pair::impl {

        vtrc::unique_ptr<thread_pool>  io_;
        thread_pool                   *rpc_;
        const bool                     same_;

        impl( unsigned thread_count )
            :io_(new thread_pool(thread_count))
            ,rpc_(io_.get( ))
            ,same_(true)
        { }

        impl( unsigned thread_count, unsigned rpc_thread_count )
            :io_(new thread_pool(thread_count))
            ,rpc_(new thread_pool(rpc_thread_count))
            ,same_(false)
        { }

        ~impl( )
        {
            io_.reset( );
            if( !same_ ) delete rpc_;
        }

        void stop_all( )
        {
            io_->stop( );
            if( !same_ ) rpc_->stop( );
        }

        void join_all( )
        {
            io_->join_all( );
            if(!same_) rpc_->join_all( );
        }

    };

    pool_pair::pool_pair( unsigned thread_count )
        :impl_(new impl(thread_count))
    { }

    pool_pair::pool_pair( unsigned thread_count, unsigned rpc_thread_count )
        :impl_(new impl(thread_count, rpc_thread_count))
    { }

    pool_pair::~pool_pair( )
    {
        delete impl_;
    }

    VTRCASIO::io_service &pool_pair::get_io_service( )
    {
        return impl_->io_->get_io_service( );
    }

    const VTRCASIO::io_service &pool_pair::get_io_service( ) const
    {
        return impl_->io_->get_io_service( );
    }

    VTRCASIO::io_service &pool_pair::get_rpc_service( )
    {
        return impl_->rpc_->get_io_service( );
    }

    const VTRCASIO::io_service &pool_pair::get_rpc_service( ) const
    {
        return impl_->rpc_->get_io_service( );
    }

    thread_pool &pool_pair::get_io_pool( )
    {
        return *impl_->io_;
    }

    const thread_pool &pool_pair::get_io_pool( ) const
    {
        return *impl_->io_;
    }

    thread_pool &pool_pair::get_rpc_pool( )
    {
        return *impl_->rpc_;
    }

    const thread_pool &pool_pair::get_rpc_pool( ) const
    {
        return *impl_->rpc_;
    }

    bool pool_pair::one_pool( ) const
    {
        return impl_->same_;
    }

    void pool_pair::stop_all( )
    {
        impl_->stop_all( );
    }

    void pool_pair::join_all( )
    {
        impl_->join_all( );
    }

}}

