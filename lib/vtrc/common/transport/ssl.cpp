#include "vtrc/common/transport/ssl.h"

#if VTRC_OPENSSL_ENABLED

#include "vtrc/common/transport/ssl.h"

#include "vtrc/common/protocol-layer.h"
#include "vtrc/common/transport/stream-impl.h"

namespace vtrc { namespace common {

    namespace basio = VTRC_ASIO;
    namespace bip   = VTRC_ASIO::ip;
    namespace bsys  = VTRC_SYSTEM;

    namespace {
        typedef transport_ssl::socket_type socket_type;
        typedef transport_impl<socket_type, transport_ssl> impl_type;

    }

    struct transport_ssl::impl: public impl_type {

        impl( vtrc::shared_ptr<socket_type> s, const std::string &n )
            :impl_type(s, n)
        { }

        void set_no_delay( bool value )
        {
            get_socket( ).lowest_layer( )
                         .set_option( bip::tcp::no_delay( value ) );
        }

        void close_handle( )
        {
            get_socket( ).lowest_layer( ).close( );
        }

    };

    transport_ssl::transport_ssl( vtrc::shared_ptr<socket_type> sock )
        :impl_(new impl(sock, "tcp"))
    {
        impl_->set_parent( this );
    }

    transport_ssl::~transport_ssl(  )
    {
        delete impl_;
    }

    std::string transport_ssl::name( ) const
    {
        return impl_->name( );
    }

    void transport_ssl::close( )
    {
        impl_->close( );
    }

    void transport_ssl::write( const char *data, size_t length )
    {
        return impl_->write( data, length );
    }

    void transport_ssl::write(const char *data, size_t length,
                     const system_closure_type &success, bool on_send_success)
    {
        impl_->write( data, length, success, on_send_success );
    }

    native_handle_type transport_ssl::native_handle( ) const
    {
        native_handle_type nh;
#ifdef _WIN32
        nh.value.win_handle =
            reinterpret_cast<native_handle_type::handle>
                ((SOCKET)impl_->get_socket( ).lowest_layer( ).native_handle( ));
#else
        nh.value.unix_fd = impl_->get_socket( ).lowest_layer( )
                                .native_handle( );
#endif
        return nh;
    }

    void transport_ssl::set_no_delay( bool value )
    {
        impl_->set_no_delay( value );
    }

    socket_type &transport_ssl::get_socket( )
    {
        return impl_->get_socket( );
    }

    const socket_type &transport_ssl::get_socket( ) const
    {
        return impl_->get_socket( );
    }

}}

#endif

