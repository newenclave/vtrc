#ifndef VTRC_CLIENT_CHANNEL_H
#define VTRC_CLIENT_CHANNEL_H

#include <google/protobuf/service.h>
#include <boost/shared_ptr.hpp>

namespace boost { namespace asio {
    class io_service;
}}

namespace vtrc { namespace client { namespace rpc {

    struct channel: public google::protobuf::RpcChannel {
        virtual ~channel( ) { }
    };

    typedef boost::shared_ptr<channel> rpc_channel_sptr;

    channel *create_channel( boost::asio::io_service &ios );

}}}

#endif
