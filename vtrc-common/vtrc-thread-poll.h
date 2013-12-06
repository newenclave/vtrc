#ifndef VTRC_THREAD_POLL_H
#define VTRC_THREAD_POLL_H

#include <stddef.h>

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc { namespace common {

    class thread_poll
    {
        struct thread_poll_impl;
        thread_poll_impl  *impl_;

        thread_poll( const thread_poll & );      // non copyable
        void operator = ( const thread_poll & ); // non copyable

    public:

        thread_poll( );
        thread_poll( size_t init_count );
        thread_poll( boost::asio::io_service &ios, size_t init_count );
        thread_poll( boost::asio::io_service &ios );
        ~thread_poll( );

        size_t size( ) const;

        void stop( );

        void join_all( );

        boost::asio::io_service &get_io_service( );
        const boost::asio::io_service &get_io_service( ) const;
    };

}}

#endif // VTRC_THREAD_POLL_H
