#ifndef LUKKI_DB_APPLICATION_H
#define LUKKI_DB_APPLICATION_H

#include "vtrc/server/application.h"
#include "vtrc/server/listener.h"

#include "vtrc/common/pool-pair.h"
#include "vtrc/common/protocol/iface.h"

#include "vtrc-function.h"

namespace google { namespace protobuf {
    class RpcController;
}}

namespace vtrc_example {
    class lukki_string_list;
    class db_stat;
    class exist_res;
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

        void execute( vtrc::common::protocol_iface::call_type call );

        void set( const std::string &name,
                  const vtrc_example::lukki_string_list &value,
                  google::protobuf::RpcController *controller );

        void upd( const std::string &name,
                  const vtrc_example::lukki_string_list &value,
                  google::protobuf::RpcController *controller );

        void get( const std::string &name,
                  vtrc_example::lukki_string_list *value,
                  google::protobuf::RpcController *controller );

        void del( const std::string &name,
                  google::protobuf::RpcController *controller );

        void stat( vtrc_example::db_stat *stat );

        void exist(const std::string &name, vtrc_example::exist_res *res );

        void subscribe_client( vtrc::common::connection_iface* conn );

    };
}

#endif // LUKKI_DB_APPLICATION_H
