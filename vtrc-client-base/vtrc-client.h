#ifndef VTRC_CLIENT_H
#define VTRC_CLIENT_H

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"
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

namespace vtrc {

    namespace common {
        class pool_pair;
        class rpc_channel;
    }

namespace client {

    class vtrc_client: public vtrc::enable_shared_from_this<vtrc_client> {

        struct impl;
        impl *impl_;

        vtrc_client( const vtrc_client &other );
        vtrc_client & operator = ( const vtrc_client &other );

    protected:

        vtrc_client( boost::asio::io_service &ios,
                     boost::asio::io_service &rpc_ios );

        vtrc_client( common::pool_pair &pools );

    public:

        ~vtrc_client( );

        common::connection_iface_sptr connection( );

        static
        vtrc::shared_ptr<vtrc_client> create( boost::asio::io_service &ios );

        static
        vtrc::shared_ptr<vtrc_client> create(boost::asio::io_service &ios,
                                             boost::asio::io_service &rpc_ios );
        static
        vtrc::shared_ptr<vtrc_client> create( common::pool_pair &pools );

        vtrc::weak_ptr<vtrc_client> weak_from_this( )
        {
            return vtrc::weak_ptr<vtrc_client>( shared_from_this( ) );
        }

        vtrc::weak_ptr<vtrc_client const> weak_from_this( ) const
        {
            return vtrc::weak_ptr<vtrc_client const>( shared_from_this( ) );
        }


        boost::asio::io_service       &get_io_service( );
        const boost::asio::io_service &get_io_service( ) const;

        boost::asio::io_service       &get_rpc_service( );
        const boost::asio::io_service &get_rpc_service( ) const;

        vtrc::shared_ptr<google::protobuf::RpcChannel> create_channel( );
        vtrc::shared_ptr<google::protobuf::RpcChannel>
                            create_channel( bool disable_wait, bool insertion );

        void connect( const std::string &local_name );
        void connect( const std::string &address, const std::string &service );

        void async_connect( const std::string &local_name,
                            common::closure_type closure);
        void async_connect( const std::string &address,
                            const std::string &service,
                            common::closure_type closure);
#ifdef _WIN32
        void connect( const std::wstring &local_name );
        void async_connect( const std::wstring &local_name,
                            common::closure_type closure);
#endif

        void advise_handler(   vtrc::shared_ptr<google::protobuf::Service> srv);
        void advise_weak_handler(vtrc::weak_ptr<google::protobuf::Service> srv);

        vtrc::shared_ptr<google::protobuf::Service> get_handler(
                                                    const std::string &name );


    };

    typedef vtrc::shared_ptr<vtrc_client> vtrc_client_sptr;
    typedef vtrc::weak_ptr<vtrc_client>   vtrc_client_wptr;

}}

#endif // VTRCCLIENT_H
