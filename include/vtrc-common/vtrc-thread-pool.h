#ifndef VTRC_THREAD_POLL_H
#define VTRC_THREAD_POLL_H

#include <stddef.h>

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc { namespace common {

    class thread_pool {

        struct impl;
        impl  *impl_;

        thread_pool( const thread_pool & );      // non copyable
        void operator = ( const thread_pool & ); // non copyable

    public:

        thread_pool( );
        thread_pool( size_t init_count );
        thread_pool( boost::asio::io_service &ios, size_t init_count );
        thread_pool( boost::asio::io_service &ios );
        ~thread_pool( );

        size_t size( ) const;
        //void interrupt_one( );
        void stop( );

        void add_thread( );
        void add_threads( size_t count );

        void join_all( );

        /// for attach current thread
        /// retrurns:
        ///     false if interrupted by interrupt_one( )
        ///     true  if stopped by stop( )
        bool attach(  );

        boost::asio::io_service       &get_io_service( );
        const boost::asio::io_service &get_io_service( ) const;
    };

}}

#endif // VTRC_THREAD_POLL_H
