#ifndef STRESS_APPLICATION_H
#define STRESS_APPLICATION_H

#include "vtrc-memory.h"

namespace vtrc { namespace server {
    class application;
}}

namespace stress {

    class application
    {
        struct                  impl;
        vtrc::unique_ptr<impl>  impl_;
    public:

        application( unsigned io_threads );
        application( unsigned io_threads, unsigned rpc_threads );

        vtrc::server::application       &get_application( );
        const vtrc::server::application &get_application( ) const;
    };

}

#endif // STRESS_APPLICATION_H
