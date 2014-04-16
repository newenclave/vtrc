#ifndef VTRC_POLL_PAIR_H
#define VTRC_POLL_PAIR_H

#include "vtrc-thread-pool.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc { namespace common {

    class thread_pool;

    class pool_pair {

        struct impl;
        impl  *impl_;

        pool_pair( const pool_pair & );
        pool_pair& operator = ( const pool_pair & );

    public:

        pool_pair( unsigned thread_count );
        pool_pair( unsigned thread_count, unsigned rpc_thread_count );
        ~pool_pair( );

    public:

        boost::asio::io_service       &get_io_service( );
        const boost::asio::io_service &get_io_service( ) const;

        boost::asio::io_service       &get_rpc_service( );
        const boost::asio::io_service &get_rpc_service( ) const;

        thread_pool       &get_io_pool( );
        const thread_pool &get_io_pool( ) const;

        thread_pool       &get_rpc_pool( );
        const thread_pool &get_rpc_pool( ) const;

        void stop_all( );
        void join_all( );

    };

}}

#endif // VTRC_POLL_PAIR_H
