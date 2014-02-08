#include "vtrc-client.h"
#include "vtrc-common/vtrc-transport-iface.h"

#include <boost/asio.hpp>

namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace gpb = google::protobuf;

//    ba::ip::tcp::resolver resolver(service());
//    ba::ip::tcp::resolver::query query(addr, serv);
//    ba::ip::tcp::resolver::iterator iterator;
//    try {
//        iterator = resolver.resolve(query);
//        handle().connect(*iterator);
//    } catch( const std::exception &ex ) {
//        ex;
//        ba::ip::tcp::endpoint ep(ba::ip::address::from_string(addr),
//                boost::lexical_cast<unsigned short>(serv) );
//        handle().connect( ep );
//    }


    struct vtrc_client::vtrc_client_impl {

        basio::io_service                         &ios_;
        vtrc_client                               *parent_;
        boost::shared_ptr<common::transport_iface> transport_;

        vtrc_client_impl( basio::io_service &ios )
            :ios_(ios)
        {}

        void connect( const std::string &address,
                      const std::string &service )
        {
//            basio::ip::tcp::socket *sock = new basio::ip::tcp::socket(ios_);
//            basio::ip::tcp::endpoint ep(ba::ip::address::from_string(address),
//                            boost::lexical_cast<unsigned short>(service) );
//            sock->connect( ep );

        }

        void async_connect( const std::string &address,
                            const std::string &service,
                            success_function   closure )
        {

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

