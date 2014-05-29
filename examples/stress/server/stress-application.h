#ifndef STRESS_APPLICATION_H
#define STRESS_APPLICATION_H

#include "vtrc-memory.h"

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
    };

}

#endif // STRESS_APPLICATION_H
