#include "vtrc-client-unix-local.h"

#ifndef _WIN32

#include "vtrc-asio.h"

#include "vtrc-client-stream-impl.h"

namespace vtrc { namespace client {

    namespace {
        typedef common::transport_unix_local::socket_type          socket_type;
        typedef client_stream_impl<client_unix_local, socket_type> super_type;
    }

    struct client_unix_local::impl: public super_type  {

        impl( VTRC_ASIO::io_service &ios,
              client::base *client, protocol_signals *callbacks )
            :super_type(ios, client, callbacks, 4096)
        { }

        void connect( const std::string &address )
        {
            basio::local::stream_protocol::endpoint ep (address);
            get_socket( ).connect( ep );
            connection_setup( );
            start_reading( );
        }

        void async_connect( const std::string &address,
                            common::system_closure_type closure )
        {
            basio::local::stream_protocol::endpoint ep (address);
            get_socket( ).async_connect( ep,
                    vtrc::bind( &this_type::on_connect, this,
                                 vtrc::placeholders::error, closure,
                                 parent_->shared_from_this( )) );
        }
    };

    static vtrc::shared_ptr<socket_type> create_socket( basio::io_service &ios )
    {
        return vtrc::make_shared<socket_type>(vtrc::ref(ios));
    }

    client_unix_local::client_unix_local( VTRC_ASIO::io_service &ios,
                                          client::base *client,
                                          protocol_signals *callbacks )
        :common::transport_unix_local(create_socket(ios))
        ,impl_(new impl(ios, client, callbacks))
    {
        impl_->set_parent( this );
    }

    vtrc::shared_ptr<client_unix_local>
        client_unix_local::create( client::base *client,
                                   protocol_signals *callbacks )
    {
        vtrc::shared_ptr<client_unix_local> new_inst
                         (new client_unix_local( client->get_io_service( ),
                                                 client, callbacks ));
        new_inst->init( );
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
                                           common::system_closure_type closure )
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

    const common::protocol_layer &client_unix_local::get_protocol( ) const
    {
        return impl_->get_protocol( );
    }


    common::environment &client_unix_local::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

    void client_unix_local::on_close( )
    {
        impl_->on_close( );
    }

    void client_unix_local::init( )
    {
        impl_->init( );
    }

    bool client_unix_local::active( ) const
    {
        return impl_->active( );
    }

    const std::string &client_unix_local::id( ) const
    {
        return impl_->id( );
    }

    void client_unix_local::raw_call_local (
                          vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                          common::empty_closure_type done )
    {
        impl_->raw_call_local( ll_mess, done );
    }


}}


#endif
