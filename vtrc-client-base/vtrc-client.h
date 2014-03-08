#ifndef VTRC_CLIENT_H
#define VTRC_CLIENT_H

#include <boost/function.hpp>

#include "vtrc-common/vtrc-connection-iface.h"

namespace boost {

namespace system {
    class error_code;
}

namespace asio {
    class io_service;
}}

namespace google { namespace protobuf {
    class RpcChannel;
}}

namespace vtrc { namespace client {

    class vtrc_client {

        struct impl;
        impl *impl_;

    public:

        vtrc_client( boost::asio::io_service &ios );
        ~vtrc_client( );
        shared_ptr<google::protobuf::RpcChannel> get_channel( );
        void connect( const std::string &address, const std::string &service );
        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type closure);

    };

}}

#endif // VTRCCLIENT_H
