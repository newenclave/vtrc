
#include "vtrc/common/transport/tcp.h"

#include "vtrc/common/protocol-layer.h"
#include "vtrc/common/transport/stream-impl.h"

namespace vtrc { namespace common {

    namespace basio = VTRC_ASIO;
    namespace bip   = VTRC_ASIO::ip;
    namespace bsys  = VTRC_SYSTEM;

    namespace {
        typedef transport_tcp::socket_type socket_type;
        typedef transport_impl<socket_type, transport_tcp> impl_type;

    }

    struct transport_tcp::impl: public impl_type {

        impl( vtrc::shared_ptr<socket_type> s, const std::string &n )
            :impl_type(s, n)
        { }

        void set_no_delay( bool value )
        {
            get_socket( ).set_option( bip::tcp::no_delay( value ) );
        }

        void close_handle( )
        {
            get_socket( ).close( );
        }

    };

    transport_tcp::transport_tcp( vtrc::shared_ptr<socket_type> sock )
        :impl_(new impl(sock, "tcp"))
    {
        impl_->set_parent( this );
    }

    transport_tcp::~transport_tcp(  )
    {
        delete impl_;
    }

    std::string transport_tcp::name( ) const
    {
        return impl_->name( );
    }

    void transport_tcp::close( )
    {
        impl_->close( );
    }

    void transport_tcp::write( const char *data, size_t length )
    {
        return impl_->write( data, length );
    }

    void transport_tcp::write(const char *data, size_t length,
                     const system_closure_type &success, bool on_send_success)
    {
        impl_->write( data, length, success, on_send_success );
    }

    native_handle_type transport_tcp::native_handle( ) const
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

    void transport_tcp::set_no_delay( bool value )
    {
        impl_->set_no_delay( value );
    }

    socket_type &transport_tcp::get_socket( )
    {
        return impl_->get_socket( );
    }

    const socket_type &transport_tcp::get_socket( ) const
    {
        return impl_->get_socket( );
    }

}}
