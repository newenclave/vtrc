
#include "vtrc-client.h"
#include "vtrc-client-tcp.h"

#include <boost/asio.hpp>

namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace gpb = google::protobuf;

    struct vtrc_client::vtrc_client_impl {

        basio::io_service                          &ios_;
        vtrc_client                                *parent_;
        boost::shared_ptr<common::connection_iface> connection_;

        vtrc_client_impl( basio::io_service &ios )
            :ios_(ios)
        {}

        void connect( const std::string &address,
                      const std::string &service )
        {
            boost::shared_ptr<client_tcp> new_client(new client_tcp( ios_ ));
            new_client->connect( address, service );
            connection_ = new_client;
        }

        void async_connect( const std::string &address,
                            const std::string &service,
                            success_function   closure )
        {
            boost::shared_ptr<client_tcp> new_client(new client_tcp( ios_ ));
            new_client->async_connect( address, service, closure );
            connection_ = new_client;
        }

    };

    vtrc_client::vtrc_client( boost::asio::io_service &ios )
        :impl_(new vtrc_client_impl(ios))
    {
        impl_->parent_ = this;
    }

    vtrc_client::~vtrc_client( )
    {
        delete impl_;
    }

    boost::shared_ptr<gpb::RpcChannel> vtrc_client::get_channel( )
    {
        return boost::shared_ptr<gpb::RpcChannel>( );
    }

    void vtrc_client::connect( const std::string &address,
                               const std::string &service )
    {
        impl_->connect( address, service );
    }

    void vtrc_client::async_connect( const std::string &address,
                            const std::string &service,
                            success_function closure )
    {
        impl_->async_connect( address, service, closure );
    }

}}

