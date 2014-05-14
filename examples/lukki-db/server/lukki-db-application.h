#ifndef LUKKI_DB_APPLICATION_H
#define LUKKI_DB_APPLICATION_H

#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-server/vtrc-listener.h"

#include "vtrc-function.h"

namespace vtrc_example {
    class lukki_string_list;
    class db_stat;
}

namespace lukki_db {

    /// operation_closure is function call after command.
    /// bool:           success flag
    /// std::string:    success message
    typedef vtrc::function<void (bool, const std::string &)> operation_closure;

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

        void set( const std::string &name,
                  const vtrc_example::lukki_string_list &value,
                  const operation_closure &closure);

        void upd( const std::string &name,
                  const vtrc_example::lukki_string_list &value,
                  const operation_closure &closure);

        void get( const std::string &name,
                  vtrc::shared_ptr<vtrc_example::lukki_string_list> value,
                  const operation_closure &closure);

        void del( const std::string &name,
                  const operation_closure &closure);

        void stat( vtrc::shared_ptr<vtrc_example::db_stat> stat,
                   const operation_closure &closure);


    };
}

#endif // LUKKI_DB_APPLICATION_H
