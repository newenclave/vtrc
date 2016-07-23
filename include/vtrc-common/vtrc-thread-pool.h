#ifndef VTRC_THREAD_POLL_H
#define VTRC_THREAD_POLL_H

#include <stddef.h>
#include "vtrc-asio-forward.h"
#include "vtrc-function.h"

VTRC_ASIO_FORWARD( class io_service; )

namespace vtrc { namespace common {

    class thread_pool {

        struct impl;
        impl  *impl_;

        thread_pool( const thread_pool & );      // non copyable
        void operator = ( const thread_pool & ); // non copyable

    public:

        typedef vtrc::function<void(void)> exception_handler;

        thread_pool(  );
        thread_pool( size_t init_count );
        thread_pool( VTRC_ASIO::io_service &ios, size_t init_count );
        thread_pool( VTRC_ASIO::io_service &ios );

        thread_pool( const char *prefix );
        thread_pool( size_t init_count, const char *prefix );
        thread_pool( VTRC_ASIO::io_service &ios,
                     size_t init_count, const char *prefix );
        thread_pool( VTRC_ASIO::io_service &ios, const char *prefix );

        ~thread_pool( );

        size_t size( ) const;
        //void interrupt_one( );
        void stop( );

        void add_thread( );
        void add_threads( size_t count );

        static const char *get_thread_prefix( );
        static void set_thread_prefix( const char *prefix );

        void join_all( );

        void assign_exception_handler( exception_handler eh );

        /// for attach current thread
        /// retrurns:
        ///     false if interrupted by interrupt_one( )
        ///     true  if stopped by stop( )
        bool attach( const char *prefix );
        bool attach(  );

        VTRC_ASIO::io_service       &get_io_service( );
        const VTRC_ASIO::io_service &get_io_service( ) const;
    };

}}

#endif // VTRC_THREAD_POLL_H
