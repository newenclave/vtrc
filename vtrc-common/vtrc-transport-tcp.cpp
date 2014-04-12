
#include "vtrc-transport-tcp.h"

#include "vtrc-protocol-layer.h"
#include "vtrc-transport-stream-impl.h"

namespace vtrc { namespace common {

    namespace basio = boost::asio;
    namespace bip   = boost::asio::ip;
    namespace bsys  = boost::system;

    namespace {
        typedef transport_tcp::socket_type socket_type;
        typedef transport_impl<socket_type, transport_tcp> impl_type;

    }

    struct transport_tcp::impl: public impl_type {

        impl( vtrc::shared_ptr<socket_type> s, const std::string &n )
            :impl_type(s, n)
        { }

        std::string prepare_for_write(const char *data, size_t len)
        {
            return get_parent( )->prepare_for_write( data, len );
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

    const char *transport_tcp::name( ) const
    {
        return impl_->name( );
    }

    void transport_tcp::close( )
    {
        impl_->close( );
    }

    common::enviroment &transport_tcp::get_enviroment( )
    {
        return impl_->get_enviroment( );
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

    std::string transport_tcp::prepare_for_write(const char *data, size_t len)
    {
        return impl_->prepare_for_write( data, len );
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
