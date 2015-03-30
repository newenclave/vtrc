#include "vtrc-transport-unix-local.h"

#ifndef  _WIN32

#include "vtrc-protocol-layer.h"
#include "vtrc-transport-stream-impl.h"

namespace vtrc { namespace common {

    namespace basio = boost::asio;
    namespace bsys  = boost::system;

    namespace {
        typedef transport_unix_local::socket_type socket_type;
        typedef transport_impl <socket_type, transport_unix_local> impl_type;
    }

    struct transport_unix_local::impl: public impl_type {

        impl( vtrc::shared_ptr<socket_type> s, const std::string &n )
            :impl_type(s, n)
        { }

        std::string prepare_for_write(const char *data, size_t len)
        {
            return get_parent( )->prepare_for_write( data, len );
        }

    };

    transport_unix_local::transport_unix_local(vtrc::shared_ptr<socket_type> s )
        :impl_(new impl(s, "unix-local"))
    {
        impl_->set_parent( this );
    }

    transport_unix_local::~transport_unix_local(  )
    {
        delete impl_;
    }

    std::string transport_unix_local::name( ) const
    {
        return impl_->name( );
    }

    void transport_unix_local::close( )
    {
        impl_->close( );
    }

    void transport_unix_local::write( const char *data, size_t length )
    {
        return impl_->write( data, length );
    }

    void transport_unix_local::write(const char *data, size_t length,
                      const system_closure_type &success, bool on_send_success)
    {
        impl_->write( data, length, success, on_send_success );
    }

    native_handle_type transport_unix_local::native_handle( ) const
    {
        native_handle_type nh;
        nh.value.unix_fd = impl_->get_socket( ).native_handle( );
        return nh;
    }

    std::string transport_unix_local::prepare_for_write(const char *data,
                                                        size_t len)
    {
        return impl_->prepare_for_write( data, len );
    }

    transport_unix_local::socket_type &transport_unix_local::get_socket( )
    {
        return impl_->get_socket( );
    }

    const transport_unix_local::socket_type
                                    &transport_unix_local::get_socket( ) const
    {
        return impl_->get_socket( );
    }

}}

#endif
