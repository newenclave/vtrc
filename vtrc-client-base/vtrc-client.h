#ifndef VTRC_CLIENT_H
#define VTRC_CLIENT_H

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-memory.h"

namespace boost {

namespace system {
    class error_code;
}

namespace asio {
    class io_service;
}}

namespace google { namespace protobuf {
    class RpcChannel;
    class Service;
}}

namespace vtrc { namespace client {

    class vtrc_client {

        struct impl;
        impl *impl_;

    public:

        vtrc_client( boost::asio::io_service &ios );
        ~vtrc_client( );
        vtrc::shared_ptr<google::protobuf::RpcChannel> get_channel( );
        void connect( const std::string &address, const std::string &service );
        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type closure);

        void advise_handler( vtrc::shared_ptr<google::protobuf::Service> serv);
        void advise_handler( vtrc::weak_ptr  <google::protobuf::Service> serv);

        vtrc::shared_ptr<google::protobuf::Service> get_handler(
                                                     const std::string &name );

    };

}}

#endif // VTRCCLIENT_H
