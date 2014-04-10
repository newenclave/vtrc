#include "vtrc-pool-pair.h"

#include "vtrc-thread.h"
#include "vtrc-mutex-typedefs.h"

//#include "boost/asio.hpp"

namespace vtrc { namespace common {

    namespace basio = boost::asio;

    struct pool_pair::impl {

        thread_pool *genegal_;
        thread_pool *rpc_;
        const bool   same_;

        impl( unsigned thread_count )
            :genegal_(new thread_pool(thread_count ? thread_count : 1))
            ,rpc_(genegal_)
            ,same_(true)
        {

        }

        impl( unsigned thread_count, unsigned rpc_thread_count )
            :genegal_(new thread_pool(thread_count ? thread_count : 1))
            ,rpc_(new thread_pool(rpc_thread_count ? rpc_thread_count : 1))
            ,same_(false)
        {

        }

        ~impl( )
        {
            stop_all( );
            join_all( );

            delete genegal_;
            if( !same_ ) delete rpc_;
        }

        void stop_all( )
        {
            genegal_->stop( );
            if( !same_ ) rpc_->stop( );
        }

        void join_all( )
        {
            genegal_->join_all( );
            if(!same_) rpc_->join_all( );
        }

    };

    pool_pair::pool_pair( unsigned thread_count )
        :impl_(new impl(thread_count))
    {}

    pool_pair::pool_pair( unsigned thread_count, unsigned rpc_thread_count )
        :impl_(new impl(thread_count, rpc_thread_count))
    {}

    pool_pair::~pool_pair( )
    {
        delete impl_;
    }

    boost::asio::io_service &pool_pair::get_io_service( )
    {
        return impl_->genegal_->get_io_service( );
    }

    const boost::asio::io_service &pool_pair::get_io_service( ) const
    {
        return impl_->genegal_->get_io_service( );
    }

    boost::asio::io_service &pool_pair::get_rpc_service( )
    {
        return impl_->rpc_->get_io_service( );
    }

    const boost::asio::io_service &pool_pair::get_rpc_service( ) const
    {
        return impl_->rpc_->get_io_service( );
    }

    thread_pool &pool_pair::get_io_pool( )
    {
        return *impl_->genegal_;
    }

    const thread_pool &pool_pair::get_io_pool( ) const
    {
        return *impl_->genegal_;
    }

    thread_pool &pool_pair::get_rpc_pool( )
    {
        return *impl_->rpc_;
    }

    const thread_pool &pool_pair::get_rpc_pool( ) const
    {
        return *impl_->rpc_;
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

