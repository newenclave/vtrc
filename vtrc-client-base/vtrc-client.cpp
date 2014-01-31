#include "vtrc-client.h"

namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace gpb = google::protobuf;

    struct vtrc_client::vtrc_client_impl {
        basio::io_service   &ios_;
        vtrc_client         *parent_;

        vtrc_client_impl( basio::io_service &ios )
            :ios_(ios)
        {}

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

    }

}}

