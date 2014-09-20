#include "vtrc-client-win-pipe.h"

#ifdef _WIN32

#include <windows.h>
#include "boost/asio/windows/stream_handle.hpp"
#include "vtrc-client-stream-impl.h"
#include "vtrc-common/vtrc-exception.h"

#include "vtrc-errors.pb.h"

namespace vtrc { namespace client {

    namespace {
        typedef common::transport_win_pipe::socket_type          socket_type;
        typedef client_stream_impl<client_win_pipe, socket_type> super_type;
    }

    struct client_win_pipe::impl: public super_type  {

        impl( boost::asio::io_service &ios,
              vtrc_client *client, protocol_signals *callbacks )
            :super_type(ios, client, callbacks, 4096)
        { }

        void connect( const std::string &address )
        {
            HANDLE pipe = CreateFileA( address.c_str( ),
                                 GENERIC_WRITE | GENERIC_READ,
                                 0,
                                 NULL, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                 NULL);

            if(INVALID_HANDLE_VALUE == pipe) {
                throw vtrc::common::exception( GetLastError( ),
                        rpc::errors::CATEGORY_SYSTEM );
            } else {
                get_socket( ).assign( pipe );
            }
            connection_setup( );
            start_reading( );
        }

        void async_connect( const std::string &address,
                            common::system_closure_type closure )
        {
            connect( address );
            bsys::error_code ec(0, bsys::get_system_category( ));
            closure(ec);
        }

        void connect( const std::wstring &address )
        {
            HANDLE pipe = CreateFileW( address.c_str( ),
                                 GENERIC_WRITE | GENERIC_READ,
                                 0,
                                 NULL, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                 NULL);

            if(INVALID_HANDLE_VALUE == pipe) {
                throw vtrc::common::exception( GetLastError( ),
                        rpc::errors::CATEGORY_SYSTEM );
            } else {
                get_socket( ).assign( pipe );
            }
            connection_setup( );
            start_reading( );
        }

        void async_connect( const std::wstring &address,
                            common::system_closure_type closure )
        {
            connect( address );
            bsys::error_code ec(0, bsys::get_system_category( ));
            closure(ec);
        }

    };

    static vtrc::shared_ptr<socket_type> create_socket( basio::io_service &ios )
    {
        return vtrc::make_shared<socket_type>(vtrc::ref(ios));
    }

    client_win_pipe::client_win_pipe( boost::asio::io_service &ios,
                                      vtrc_client *client, 
                                      protocol_signals *callbacks )
        :common::transport_win_pipe(create_socket(ios))
        ,impl_(new impl(ios, client, callbacks ))
    {
        impl_->set_parent( this );
    }

    vtrc::shared_ptr<client_win_pipe> client_win_pipe::create(
                                 basio::io_service &ios, 
                                 vtrc_client *client, 
                                 protocol_signals *callbacks)
    {
        vtrc::shared_ptr<client_win_pipe> new_inst
                    (new client_win_pipe( ios, client, callbacks ));

        new_inst->init( );
        return new_inst;
    }

    client_win_pipe::~client_win_pipe( )
    {
        delete impl_;
    }

    void client_win_pipe::connect( const std::string &address )
    {
        impl_->connect( address );
    }

    void client_win_pipe::connect( const std::wstring &address )
    {
        impl_->connect( address );
    }

    void client_win_pipe::async_connect( const std::string &address,
                                   common::system_closure_type  closure )
    {
        impl_->async_connect( address, closure );
    }

    void client_win_pipe::async_connect( const std::wstring &address,
                                 common::system_closure_type  closure )
    {
        impl_->async_connect( address, closure );
    }

    const common::call_context *client_win_pipe::get_call_context( ) const
    {
        return impl_->get_call_context( );
    }

    void client_win_pipe::on_write_error( const bsys::error_code &err )
    {
        impl_->on_write_error( err );
        this->close( );
    }

    common::protocol_layer &client_win_pipe::get_protocol( )
    {
        return impl_->get_protocol( );
    }

    common::enviroment &client_win_pipe::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

    std::string client_win_pipe::prepare_for_write(
                                                  const char *data, size_t len)
    {
        return impl_->prepare_for_write( data, len );
    }

    bool client_win_pipe::active( ) const
    {
        return impl_->active( );
    }

    void client_win_pipe::init( )
    {
        impl_->init( );
    }

    void client_win_pipe::on_close( )
    {
        impl_->on_close( );
    }

    const std::string &client_win_pipe::id( ) const
    {
        return impl_->id( );
    }

}}


#endif
