#include <boost/asio.hpp>

#include "vtrc-client.h"
#include "vtrc-client-tcp.h"

#include "vtrc-rpc-channel.h"
#include "vtrc-bind.h"

namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;
    namespace gpb = google::protobuf;

    struct vtrc_client::impl {

        typedef impl this_type;

        basio::io_service              &ios_;
        vtrc_client                    *parent_;
        common::connection_iface_sptr   connection_;
        vtrc::shared_ptr<rpc_channel>         channel_;

        impl( basio::io_service &ios )
            :ios_(ios)
        {}

        void connect( const std::string &address,
                      const std::string &service )
        {
            vtrc::shared_ptr<client_tcp>
                               new_client(client_tcp::create( ios_, parent_ ));
            new_client->connect( address, service );
            connection_ = new_client;
            channel_ = vtrc::make_shared<rpc_channel>( connection_ );
        }

        void async_connect_success( const bsys::error_code &err,
                                    common::closure_type closure )
        {
            if( !err )
                channel_ = vtrc::make_shared<rpc_channel>( connection_ );
            closure(err);
        }

        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type &closure )
        {
            vtrc::shared_ptr<client_tcp>
                               new_client(client_tcp::create( ios_ , parent_));

            new_client->async_connect( address, service,
                        vtrc::bind( &this_type::async_connect_success, this,
                                     basio::placeholders::error,
                                     closure ));
            connection_ = new_client;
        }

        vtrc::shared_ptr<gpb::RpcChannel> get_channel( )
        {
            return channel_;
        }

    };

    vtrc_client::vtrc_client( boost::asio::io_service &ios )
        :impl_(new impl(ios))
    {
        impl_->parent_ = this;
    }

    vtrc_client::~vtrc_client( )
    {
        delete impl_;
    }

    vtrc::shared_ptr<gpb::RpcChannel> vtrc_client::get_channel( )
    {
        return impl_->get_channel( );
    }

    void vtrc_client::connect( const std::string &address,
                               const std::string &service )
    {
        impl_->connect( address, service );
    }

    void vtrc_client::async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type closure )
    {
        impl_->async_connect( address, service, closure );
    }

}}

