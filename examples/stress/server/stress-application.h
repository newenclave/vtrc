#ifndef STRESS_APPLICATION_H
#define STRESS_APPLICATION_H

#include "vtrc-memory.h"
#include "vtrc-function.h"

namespace vtrc { namespace server {
    class application;
}}

namespace boost { namespace program_options {
    class variables_map;
}}

namespace stress {

    class application
    {
        struct   impl;
        impl    *impl_;

    public:

        application( unsigned io_threads );
        application( unsigned io_threads, unsigned rpc_threads );

        ~application( );

    public:

        vtrc::server::application       &get_application( );
        const vtrc::server::application &get_application( ) const;

        void run( const boost::program_options::variables_map &params );

        void stop( );

        typedef vtrc::function<bool (unsigned, unsigned)> timer_callback;

        unsigned add_timer_event( unsigned microseconds, timer_callback );
        void del_timer_event( unsigned id );

    };

}

#endif // STRESS_APPLICATION_H
