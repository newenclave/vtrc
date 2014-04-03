#include "vtrc-client-unix-local.h"

#ifndef _WIN32

#include <boost/asio/local/stream_protocol.hpp>

#include "vtrc-client-stream-impl.h"

namespace vtrc { namespace client {

    namespace {
        typedef common::transport_unix_local::socket_type          socket_type;
        typedef client_stream_impl<client_unix_local, socket_type> super_type;
    }

    struct client_unix_local::impl: public super_type  {

        impl( boost::asio::io_service &ios, vtrc_client *client )
            :super_type(ios, client, 4096)
        { }

        void connect( const std::string &address )
        {
            basio::local::stream_protocol::endpoint ep (address);
            get_socket( ).connect( ep );
            init( );
        }

        void async_connect( const std::string &address,
                            common::closure_type closure )
        {
            basio::local::stream_protocol::endpoint ep (address);
            get_socket( ).async_connect( ep,
                    vtrc::bind( &this_type::on_connect, this,
                                 basio::placeholders::error, closure,
                                 parent_->shared_from_this( )) );
        }
    };

    static vtrc::shared_ptr<socket_type> create_socket( basio::io_service &ios )
    {
        return vtrc::make_shared<socket_type>(vtrc::ref(ios));
    }

    client_unix_local::client_unix_local( boost::asio::io_service &ios,
                                                        vtrc_client *client )
        :common::transport_unix_local(create_socket(ios))
        ,impl_(new impl(ios, client))
    {
        impl_->set_parent( this );
    }

    vtrc::shared_ptr<client_unix_local> client_unix_local::create(
                                 basio::io_service &ios, vtrc_client *client)
    {
        vtrc::shared_ptr<client_unix_local> new_inst
                                        (new client_unix_local( ios, client ));
        return new_inst;
    }

    client_unix_local::~client_unix_local( )
    {
        delete impl_;
    }

    void client_unix_local::connect( const std::string &address )
    {
        impl_->connect( address );
    }

    void client_unix_local::async_connect( const std::string &address,
                                           common::closure_type  closure )
    {
        impl_->async_connect( address, closure );
    }

    void client_unix_local::on_write_error( const bsys::error_code &err )
    {
        impl_->on_write_error( err );
        this->close( );
    }

    const common::call_context *client_unix_local::get_call_context( ) const
    {
        return impl_->get_call_context( );
    }

    common::protocol_layer &client_unix_local::get_protocol( )
    {
        return impl_->get_protocol( );
    }

    std::string client_unix_local::prepare_for_write(
                                                  const char *data, size_t len)
    {
        return impl_->prepare_for_write( data, len );
    }

    void client_unix_local::init( )
    {

    }

    bool client_unix_local::active( ) const
    {
        return impl_->active( );
    }

}}


#endif
