#ifndef LUKKI_DB_APPLICATION_H
#define LUKKI_DB_APPLICATION_H

#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-pool-pair.h"

namespace lukki_db {

    class application: vtrc::server::application {
        struct  impl;
        impl   *impl_;
    public:
        application( vtrc::common::pool_pair &pp );
        ~application( );
    };
}

#endif // LUKKI_DB_APPLICATION_H
