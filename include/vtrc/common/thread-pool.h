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

        enum call_decorator_type { CALL_PROLOGUE, CALL_EPILOGUE };

        typedef vtrc::function<void(call_decorator_type)> thread_decorator;
        typedef vtrc::function<void(void)> exception_handler;

        thread_pool(  );
        thread_pool( size_t init_count );
        thread_pool( VTRC_ASIO::io_service &ios, size_t init_count );
        thread_pool( VTRC_ASIO::io_service &ios );

        ~thread_pool( );

        size_t size( ) const;
        //void interrupt_one( );
        void stop( );

        void add_thread( );
        void add_threads( size_t count );

        void join_all( );

        void assign_exception_handler( exception_handler eh );
        void assign_thread_decorator( thread_decorator td );

        /// for attach current thread
        /// retrurns:
        ///     false if interrupted by interrupt_one( )
        ///     true  if stopped by stop( )
        bool attach(  );
        bool attach( thread_decorator td );

        VTRC_ASIO::io_service       &get_io_service( );
        const VTRC_ASIO::io_service &get_io_service( ) const;
    };

}}

#endif // VTRC_THREAD_POLL_H
