#ifndef VTRC_CLIENT_H
#define VTRC_CLIENT_H

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

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

    typedef boost::function <
        void (const boost::system::error_code &)
    > success_function;

    class vtrc_client {

        struct vtrc_client_impl;
        vtrc_client_impl *impl_;

    public:

        vtrc_client( boost::asio::io_service &ios );
        ~vtrc_client( );
        boost::shared_ptr<google::protobuf::RpcChannel> get_channel( );
        void connect( const std::string &address, const std::string &service );
        void async_connect( const std::string &address,
                            const std::string &service,
                            success_function   closure);

    };

}}

#endif // VTRCCLIENT_H
