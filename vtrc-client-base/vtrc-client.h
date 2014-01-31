#ifndef VTRC_CLIENT_H
#define VTRC_CLIENT_H

#include <boost/shared_ptr.hpp>

namespace boost { namespace asio {
    class io_service;
}}

namespace google { namespace protobuf {
    class RpcChannel;
}}

namespace vtrc { namespace client {

    class vtrc_client {

        struct vtrc_client_impl;
        vtrc_client_impl *impl_;

    public:

        vtrc_client( boost::asio::io_service &ios );
        ~vtrc_client( )
        boost::shared_ptr<google::protobuf::RpcChannel> get_channel( );
        void connect( const std::string &address, const std::string &service );

    };

}}

#endif // VTRCCLIENT_H
