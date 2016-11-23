#ifndef VTRC_TRANSPORT_DGRAM_IMPL_H
#define VTRC_TRANSPORT_DGRAM_IMPL_H

#include <vector>

#include "vtrc-asio.h"
#include "vtrc-system.h"
#include "vtrc-memory.h"
#include "vtrc-function.h"
#include "vtrc-bind.h"
#include "vtrc-stdint.h"

#include "vtrc-common/vtrc-closure.h"

namespace vtrc { namespace common {

    namespace basio = VTRC_ASIO;
    namespace bsys  = VTRC_SYSTEM;

    struct dgram_transport_impl {

        typedef dgram_transport_impl this_type;

        basio::ip::udp::socket      sock_;
        basio::io_service::strand   dispatcher_;
        std::vector<std::uint8_t>   data_;

        struct message_holder {
            std::string                 message_;
            common::system_closure_type closure_;
        };
        typedef vtrc::shared_ptr<message_holder> message_holder_sptr;

        static void default_closure( const bsys::error_code & ) { }

        static
        message_holder_sptr make_holder( const char *data, size_t length,
                                         system_closure_type closure )
        {
            /// TODO: FIX IT
            message_holder_sptr mh(vtrc::make_shared<message_holder>());
            mh->message_.assign( data, data + length );
            mh->closure_ = closure;
            return mh;
        }

        dgram_transport_impl( basio::io_service &ios, size_t buflen )
            :sock_(ios)
            ,dispatcher_(ios)
            ,data_(buflen ? buflen : 4096)
        { }

        void close( )
        {
            sock_.close( );
        }

        void write_handler( const bsys::error_code &err, size_t len,
                            message_holder_sptr mess )
        {
            mess->closure_( err );
            on_write( err, mess->message_.size( ) );
        }

        void read_handler( const bsys::error_code &err, std::size_t len )
        {
            const static basio::ip::udp::endpoint empty;
            on_read( err, empty, &data_[0], len );
        }

        void read_handler2( const bsys::error_code &err,
                            std::size_t len,
                            std::shared_ptr<basio::ip::udp::endpoint> from )
        {
            on_read( err, *from, &data_[0], len );
        }

        void set_buf_size( vtrc::uint16_t len )
        {
            data_.resize( len );
            basio::socket_base::send_buffer_size    snd( len );
            basio::socket_base::receive_buffer_size rcv( len );
            bsys::error_code ec;
            sock_.set_option( snd, ec );
            sock_.set_option( rcv, ec );
        }

        const std::vector<std::uint8_t> &get_data( ) const
        {
            return data_;
        }

        void set_buffer_size( vtrc::uint16_t len )
        {
            dispatch( std::bind( &this_type::set_buf_size, this, len ) );
        }

        void open_v4( )
        {
            sock_.open( basio::ip::udp::v4( ) );
        }

        void open_v6( )
        {
            sock_.open( basio::ip::udp::v6( ) );
        }

        basio::io_service &get_io_service( )
        {
            return sock_.get_io_service( );
        }

        void dispatch( vtrc::function<void ()> call )
        {
            dispatcher_.dispatch( call );
        }

        basio::ip::udp::socket &get_socket( )
        {
            return sock_;
        }

        void write( const char *data, size_t len,
                    basio::socket_base::message_flags flags = 0 )
        {
            namespace ph = vtrc::placeholders;
            message_holder_sptr mh = make_holder( data, len,
                                                  this_type::default_closure );
            sock_.async_send( basio::buffer(data, len),
                flags,
                dispatcher_.wrap(
                   vtrc::bind( &this_type::write_handler,
                               this, ph::_1, ph::_2, mh )
                ) );
        }

        void write_to( const char *data, size_t len,
                       const basio::ip::udp::endpoint &to,
                       basio::socket_base::message_flags flags = 0)
        {
            namespace ph = vtrc::placeholders;
            message_holder_sptr mh = make_holder( data, len,
                                                  this_type::default_closure );
            sock_.async_send_to( basio::buffer(data, len), to,
                flags,
                dispatcher_.wrap(
                   vtrc::bind( &this_type::write_handler, this,
                               ph::_1, ph::_2, mh )
                ) );
        }

        void read( basio::socket_base::message_flags flags = 0 )
        {
            namespace ph = vtrc::placeholders;
            sock_.async_receive( basio::buffer(&data_[0], data_.size( )),
                    flags,
                    dispatcher_.wrap(
                        vtrc::bind( &this_type::read_handler, this,
                                     ph::_1, ph::_2 )
                    ) );
        }

        void read_from( const basio::ip::udp::endpoint &from,
                        basio::socket_base::message_flags flags = 0 )
        {
            namespace ph = vtrc::placeholders;
            vtrc::shared_ptr<basio::ip::udp::endpoint> ep
                    = vtrc::make_shared<basio::ip::udp::endpoint>(from);
            sock_.async_receive_from( basio::buffer(&data_[0], data_.size( ) ),
                            *ep, flags,
                            dispatcher_.wrap(
                                vtrc::bind( &this_type::read_handler2, this,
                                            ph::_1, ph::_2, ep )
                            ) );
        }

        virtual void on_write( const bsys::error_code &, std::size_t ) { }
        virtual void start( ) { }
        virtual void on_read( const bsys::error_code &,
                              const basio::ip::udp::endpoint &,
                              std::uint8_t *, std::size_t ) { }

    };

}}

#endif // VTRC_TRANSPORT_DGRAM_IMPL_H
