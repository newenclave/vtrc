#ifndef LUKKI_DB_APPLICATION_H
#define LUKKI_DB_APPLICATION_H

#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-server/vtrc-listener.h"

namespace lukki_db {

    class application: public vtrc::server::application {
        struct  impl;
        impl   *impl_;
    public:
        application( vtrc::common::pool_pair &pp );
        ~application( );

        void attach_start_listener( vtrc::server::listener_sptr listen );

        vtrc::shared_ptr<vtrc::common::rpc_service_wrapper>
                 get_service_by_name( vtrc::common::connection_iface* conn,
                                      const std::string &service_name );
    };
}

#endif // LUKKI_DB_APPLICATION_H
