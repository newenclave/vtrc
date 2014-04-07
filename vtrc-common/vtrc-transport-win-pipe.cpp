
#include "vtrc-transport-win-pipe.h"

#if 1
#ifdef _WIN32

#include <windows.h>
#include "vtrc-transport-stream-impl.h"

namespace vtrc { namespace common {

    namespace basio = boost::asio;
    namespace bip   = boost::asio::ip;
    namespace bsys  = boost::system;

    namespace {
        typedef transport_win_pipe::socket_type socket_type;
        typedef transport_impl<socket_type, transport_win_pipe> impl_type;
    }

    struct transport_win_pipe::impl: public impl_type {

        impl( vtrc::shared_ptr<socket_type> s, const std::string &n )
            :impl_type(s, n)
        {  }

        std::string prepare_for_write(const char *data, size_t len)
        {
            return get_parent( )->prepare_for_write( data, len );
        }

    };

    transport_win_pipe::transport_win_pipe( vtrc::shared_ptr<socket_type> sock )
        :impl_(new impl(sock, "win-pipe"))
    {
        impl_->set_parent( this );
    }

    transport_win_pipe::~transport_win_pipe(  )
    {
        delete impl_;
    }

    const char *transport_win_pipe::name( ) const
    {
        return impl_->name( );
    }

    void transport_win_pipe::close( )
    {
        impl_->close( );
    }

    common::enviroment &transport_win_pipe::get_enviroment( )
    {
        return impl_->get_enviroment( );
    }

    void transport_win_pipe::write( const char *data, size_t length )
    {
        return impl_->write( data, length );
    }

    void transport_win_pipe::write(const char *data, size_t length,
                     const closure_type &success, bool on_send_success)
    {
        impl_->write( data, length, success, on_send_success );
    }

    std::string transport_win_pipe::prepare_for_write(const char *data,
                                                             size_t len)
    {
        return impl_->prepare_for_write( data, len );
    }

    socket_type &transport_win_pipe::get_socket( )
    {
        return impl_->get_socket( );
    }

    const socket_type &transport_win_pipe::get_socket( ) const
    {
        return impl_->get_socket( );
    }


}}


#endif
#endif
