
#include "vtrc-client.h"
#include "vtrc-client-tcp.h"

#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

#include "vtrc-rpc-channel.h"

namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;
    namespace gpb = google::protobuf;

    struct vtrc_client::vtrc_client_impl {

        typedef vtrc_client_impl this_type;

        basio::io_service                          &ios_;
        vtrc_client                                *parent_;
        boost::shared_ptr<common::connection_iface> connection_;
        boost::shared_ptr<rpc_channel>              channel_;

        vtrc_client_impl( basio::io_service &ios )
            :ios_(ios)
        {}

        void connect( const std::string &address,
                      const std::string &service )
        {
            boost::shared_ptr<client_tcp>
                               new_client(client_tcp::create( ios_, parent_ ));
            new_client->connect( address, service );
            connection_ = new_client;
            channel_ = boost::make_shared<rpc_channel>( connection_ );
        }


        void async_connect_success( const bsys::error_code &err,
                                    common::closure_type closure )
        {
            if( !err )
                channel_ = boost::make_shared<rpc_channel>( connection_ );
            closure(err);
        }

        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type &closure )
        {
            boost::shared_ptr<client_tcp>
                               new_client(client_tcp::create( ios_ , parent_));

            new_client->async_connect( address, service,
                        boost::bind( &this_type::async_connect_success, this,
                                     basio::placeholders::error,
                                     closure ));
            connection_ = new_client;
        }

        boost::shared_ptr<gpb::RpcChannel> get_channel( )
        {
            return channel_;
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
        return impl_->get_channel( );
    }

    void vtrc_client::connect( const std::string &address,
                               const std::string &service )
    {
        impl_->connect( address, service );
    }

    void vtrc_client::async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type &closure )
    {
        impl_->async_connect( address, service, closure );
    }

}}

