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

    class vtrc_client: public vtrc::enable_shared_from_this<vtrc_client> {

        struct impl;
        impl *impl_;

        vtrc_client( const vtrc_client &other );
        vtrc_client & operator = ( const vtrc_client &other );

        vtrc_client( boost::asio::io_service &ios );

    public:

        static
        vtrc::shared_ptr<vtrc_client> create( boost::asio::io_service &ios );

        vtrc::weak_ptr<vtrc_client> weak_from_this( )
        {
            return vtrc::weak_ptr<vtrc_client>( shared_from_this( ) );
        }

        vtrc::weak_ptr<vtrc_client const> weak_from_this( ) const
        {
            return vtrc::weak_ptr<vtrc_client const>( shared_from_this( ) );
        }

        ~vtrc_client( );

        boost::asio::io_service       &get_io_service( );
        const boost::asio::io_service &get_io_service( ) const;

        vtrc::shared_ptr<google::protobuf::RpcChannel> get_channel( );
        void connect( const std::string &address, const std::string &service );
        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type closure);

        void advise_handler( vtrc::shared_ptr<google::protobuf::Service> serv);
        void advise_weak_handler(vtrc::weak_ptr<google::protobuf::Service> ser);

        vtrc::shared_ptr<google::protobuf::Service> get_handler(
                                                    const std::string &name );


    };

    typedef vtrc::shared_ptr<vtrc_client> vtrc_client_sptr;
    typedef vtrc::weak_ptr<vtrc_client>   vtrc_client_wptr;

}}

#endif // VTRCCLIENT_H
