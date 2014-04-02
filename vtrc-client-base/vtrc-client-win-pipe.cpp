#include "vtrc-client-win-pipe.h"

#ifdef _WIN32

#include <boost/asio/windows/stream_handle.hpp>
#include "vtrc-client-stream-impl.h"

namespace vtrc { namespace client {

    namespace {
        typedef common::transport_win_pipe::socket_type          socket_type;
        typedef client_stream_impl<client_win_pipe, socket_type> super_type;
    }

    struct client_win_pipe::impl: public super_type  {

        impl( boost::asio::io_service &ios, vtrc_client *client )
            :super_type(ios, client, 4096)
        { }

        void connect( const std::string &address )
        {
            init( );
        }

        void async_connect( const std::string &address,
                            common::closure_type closure )
        {

        }

    };

    static vtrc::shared_ptr<socket_type> create_socket( basio::io_service &ios )
    {
        return vtrc::make_shared<socket_type>(vtrc::ref(ios));
    }

    client_win_pipe::client_win_pipe( boost::asio::io_service &ios,
                                                        vtrc_client *client )
        :common::transport_win_pipe(create_socket(ios))
        ,impl_(new impl(ios, client))
    {
        impl_->set_parent( this );
    }

    vtrc::shared_ptr<client_win_pipe> client_win_pipe::create(
                                 basio::io_service &ios, vtrc_client *client)
    {
        vtrc::shared_ptr<client_win_pipe> new_inst
                                        (new client_win_pipe( ios, client ));
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

    void client_win_pipe::async_connect( const std::string &address,
                                           common::closure_type  closure )
    {
        impl_->async_connect( address, closure );
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

    std::string client_win_pipe::prepare_for_write(
                                                  const char *data, size_t len)
    {
        return impl_->prepare_for_write( data, len );
    }

    void client_win_pipe::init( )
    {

    }

}}


#endif
