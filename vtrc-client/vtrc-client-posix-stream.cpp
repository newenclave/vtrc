#include "vtrc-client-posix-stream.h"

#ifndef _WIN32

#include "vtrc-asio.h"
#include "vtrc-client-stream-impl.h"
#include "vtrc-common/vtrc-exception.h"

#include <error.h>

namespace vtrc { namespace client {

    namespace {
        typedef common::transport_posixs::socket_type          socket_type;
        typedef client_stream_impl<client_posixs, socket_type> super_type;
    }

    struct client_posixs::impl: public super_type  {

        impl( VTRC_ASIO::io_service &ios,
              vtrc_client *client, protocol_signals *callbacks )
            :super_type(ios, client, callbacks, 4096)
        { }

        void connect( const std::string &address, int flags, int mode )
        {
            typedef socket_type::native_handle_type fd_type;
            fd_type fd = open( address.c_str( ), flags | O_NONBLOCK, mode );
            if( -1 == fd ) {
                VTRC_SYSTEM::error_code ec( errno,
                                            VTRC_SYSTEM::get_posix_category( ));
                common::raise( std::runtime_error( ec.message( ) ) );
                return;
            }
            get_socket( ).assign( fd );
            connection_setup( );
            start_reading( );
        }

        void async_connect( const std::string &address,
                            int flags, int mode,
                            common::system_closure_type closure )
        {
            typedef socket_type::native_handle_type fd_type;
            fd_type fd = open( address.c_str( ), flags | O_NONBLOCK, mode );
            if( -1 == fd ) {
                VTRC_SYSTEM::error_code ec( errno,
                                            VTRC_SYSTEM::get_posix_category( ));
                closure( ec );
                return;
            }
            get_socket( ).assign( fd );
            closure( VTRC_SYSTEM::error_code( ) );
        }
    };

    static vtrc::shared_ptr<socket_type> create_socket( basio::io_service &ios )
    {
        return vtrc::make_shared<socket_type>(vtrc::ref(ios));
    }

    client_posixs::client_posixs( VTRC_ASIO::io_service &ios,
                                          vtrc_client *client,
                                          protocol_signals *callbacks )
        :common::transport_posixs(create_socket(ios))
        ,impl_(new impl(ios, client, callbacks))
    {
        impl_->set_parent( this );
    }

    vtrc::shared_ptr<client_posixs>
        client_posixs::create(basio::io_service &ios,
                                  vtrc_client *client,
                                  protocol_signals *callbacks )
    {
        vtrc::shared_ptr<client_posixs> new_inst
                         (new client_posixs( ios, client, callbacks ));
        new_inst->init( );
        return new_inst;
    }

    client_posixs::~client_posixs( )
    {
        delete impl_;
    }

    void client_posixs::connect( const std::string &path, int flags, int mode )
    {
        impl_->connect( path, flags, mode );
    }

    void client_posixs::async_connect( const std::string &address,
                                       int flags, int mode,
                                       common::system_closure_type closure )
    {
        impl_->async_connect( address, flags, mode, closure );
    }

    void client_posixs::on_write_error( const bsys::error_code &err )
    {
        impl_->on_write_error( err );
        this->close( );
    }

    const common::call_context *client_posixs::get_call_context( ) const
    {
        return impl_->get_call_context( );
    }

    common::protocol_layer &client_posixs::get_protocol( )
    {
        return impl_->get_protocol( );
    }

    const common::protocol_layer &client_posixs::get_protocol( ) const
    {
        return impl_->get_protocol( );
    }


    common::enviroment &client_posixs::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

    void client_posixs::on_close( )
    {
        impl_->on_close( );
    }

    void client_posixs::init( )
    {
        impl_->init( );
    }

    bool client_posixs::active( ) const
    {
        return impl_->active( );
    }

    const std::string &client_posixs::id( ) const
    {
        return impl_->id( );
    }

    void client_posixs::raw_call_local (
                          vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess,
                          common::empty_closure_type done )
    {
        impl_->raw_call_local( ll_mess, done );
    }

}}

#endif // _WIN32
