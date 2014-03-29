#include "vtrc-client-unix-local.h"

#ifndef _WIN32

#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>

#include "vtrc-protocol-layer-c.h"

#include "vtrc-client.h"
#include "vtrc-bind.h"
#include "vtrc-ref.h"
#include "vtrc-chrono.h"

namespace vtrc { namespace client {

    namespace basio = boost::asio;
    namespace bsys = boost::system;


    namespace {
        typedef common::transport_unix_local::socket_type socket_type;
    }

    struct client_unix_local::impl  {

        typedef impl this_type;

        boost::asio::io_service &ios_;
        client_unix_local       *parent_;
        std::vector<char>        read_buff_;

        vtrc_client             *client_;

        vtrc::shared_ptr<protocol_layer_c> protocol_;

        impl( boost::asio::io_service &ios, vtrc_client *client )
            :ios_(ios)
            ,read_buff_(4096)
            ,client_(client)
        {

        }

        socket_type &sock( )
        {
            return parent_->get_socket( );
        }

        void init(  )
        {
            protocol_.reset(new client::protocol_layer_c( parent_, client_ ));
            start_reading( );
        }

        void connect( const std::string &address )
        {
            basio::local::stream_protocol::endpoint ep (address);
            sock( ).connect( ep );
            init( );
        }

        void on_connect( const boost::system::error_code &err,
                         common::closure_type closure,
                         common::connection_iface_sptr parent)
        {
            if( !err ) {
                init( );
            }
            closure( err );
        }

        void async_connect( const std::string &address,
                            common::closure_type closure )
        {
            basio::local::stream_protocol::endpoint ep (address);
            sock( ).async_connect( ep,
                    vtrc::bind( &this_type::on_connect, this,
                                 basio::placeholders::error, closure,
                                 parent_->shared_from_this( )) );
        }

        void start_reading( )
        {
#if 0
            basio::io_service::strand &disp(parent_->get_dispatcher( ));

            sock( ).async_read_some(
                    basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                    disp.wrap(vtrc::bind( &this_type::read_handler, this,
                         basio::placeholders::error,
                         basio::placeholders::bytes_transferred,
                         parent_->weak_from_this( ) ))
                );
#else
            sock( ).async_read_some(
                    basio::buffer( &read_buff_[0], read_buff_.size( ) ),
                        vtrc::bind( &this_type::read_handler, this,
                             basio::placeholders::error,
                             basio::placeholders::bytes_transferred,
                             parent_->weak_from_this( ) )
                );
#endif
        }

        void read_handler( const bsys::error_code &error, size_t bytes,
                           common::connection_iface_wptr parent)
        {
            common::connection_iface_sptr lk(parent.lock( ));
            if( !lk ) return;

            if( !error ) {
                try {
                    protocol_->process_data( &read_buff_[0], bytes );
                } catch( const std::exception & /*ex*/ ) {
                    parent_->close( );
                    return;
                }
                start_reading( );
            } else {
                protocol_->on_read_error( error );
                parent_->close( );
            }
        }

        std::string prepare_for_write(const char *data, size_t len)
        {
            return protocol_->prepare_data( data, len );
        }

        common::protocol_layer &get_protocol( )
        {
            return *protocol_;
        }

        void on_write_error( const boost::system::error_code &err )
        {
            protocol_->on_write_error( err );
        }

    };

    static vtrc::shared_ptr<socket_type> create_socket( basio::io_service &ios )
    {
        return vtrc::make_shared<socket_type>(vtrc::ref(ios));
    }

    client_unix_local::client_unix_local( boost::asio::io_service &ios,
                                                        vtrc_client *client )
        :common::transport_unix_local(create_socket(ios))
        ,impl_(new impl(ios, client))
    {
        impl_->parent_ = this;
    }

    vtrc::shared_ptr<client_unix_local> client_unix_local::create(
                                 basio::io_service &ios, vtrc_client *client)
    {
        vtrc::shared_ptr<client_unix_local> new_inst
                                        (new client_unix_local( ios, client ));
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
                                    vtrc::function <
                                        void (const boost::system::error_code &)
                                    >   closure )
    {
        impl_->async_connect( address, closure );
    }

    void client_unix_local::on_write_error(
                                         const boost::system::error_code &err )
    {
        impl_->on_write_error( err );
        this->close( );
    }

    common::protocol_layer &client_unix_local::get_protocol( )
    {
        return impl_->get_protocol( );
    }

    std::string client_unix_local::prepare_for_write(
                                                  const char *data, size_t len)
    {
        return impl_->prepare_for_write( data, len );
    }

    void client_unix_local::init( )
    {

    }

}}


#endif
