#include <iostream>

#include "vtrc-transport-posix-stream.h"

#include "vtrc-protocol-layer.h"
#include "vtrc-transport-stream-impl.h"

namespace vtrc { namespace common {

    namespace basio = VTRC_ASIO;
    namespace bip   = VTRC_ASIO::ip;
    namespace bsys  = VTRC_SYSTEM;

    namespace {
        typedef transport_posixs::socket_type socket_type;
        typedef transport_impl<socket_type, transport_posixs> impl_type;

    }

    struct transport_posixs::impl: public impl_type {

        impl( vtrc::shared_ptr<socket_type> s, const std::string &n )
            :impl_type(s, n)
        { }

        void close_handle( )
        {
            get_socket( ).close( );
        }

    };

    transport_posixs::transport_posixs( vtrc::shared_ptr<socket_type> sock )
        :impl_(new impl(sock, "tcp"))
    {
        impl_->set_parent( this );
    }

    transport_posixs::~transport_posixs(  )
    {
        delete impl_;
    }

    std::string transport_posixs::name( ) const
    {
        return impl_->name( );
    }

    void transport_posixs::close( )
    {
        impl_->close( );
    }

    void transport_posixs::write( const char *data, size_t length )
    {
        return impl_->write( data, length );
    }

    void transport_posixs::write(const char *data, size_t length,
                     const system_closure_type &success, bool on_send_success)
    {
        impl_->write( data, length, success, on_send_success );
    }

    native_handle_type transport_posixs::native_handle( ) const
    {
        native_handle_type nh;
#ifdef _WIN32
        nh.value.win_handle =
            reinterpret_cast<native_handle_type::handle>
                ((SOCKET)impl_->get_socket( ).native_handle( ));
#else
        nh.value.unix_fd = impl_->get_socket( ).native_handle( );
#endif
        return nh;
    }

    socket_type &transport_posixs::get_socket( )
    {
        return impl_->get_socket( );
    }

    const socket_type &transport_posixs::get_socket( ) const
    {
        return impl_->get_socket( );
    }

}}

