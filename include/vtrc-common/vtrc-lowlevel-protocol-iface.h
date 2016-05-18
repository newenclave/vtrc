#ifndef VTRC_lowlevel_protocol_layer_IFACE_H
#define VTRC_lowlevel_protocol_layer_IFACE_H

#include <string>
#include "vtrc-closure.h"
#include "vtrc-memory.h"
#include "vtrc-function.h"
#include "vtrc-protocol-accessor-iface.h"

namespace google { namespace protobuf {
    class MessageLite;
    class Message;
}}

namespace vtrc { namespace rpc {
    class session_options;
    class lowlevel_unit;
}}

namespace vtrc { namespace common {

    namespace lowlevel {

    struct protocol_layer_iface {
        virtual ~protocol_layer_iface( ) { }

        /// init current protocol
        virtual void init( protocol_accessor *pa,
                           system_closure_type ready_for_read ) = 0;

        /// close connection
        virtual void close( ) = 0;

        /// do handshake with other side
        virtual void do_handshake( ) = 0;

        /// serialize lowlevel message
        virtual std::string pack_message( const rpc::lowlevel_unit &mess ) = 0;

        /// accept portion os the data and unpuck message is exists
        virtual void process_data( const char *data, size_t length ) = 0;

        /// ready messages count
        virtual size_t queue_size( ) const = 0;

        /// get and pop next message
        virtual bool pop_proto_message( rpc::lowlevel_unit &result ) = 0;

    };

    typedef vtrc::shared_ptr<protocol_layer_iface> protocol_layer_sptr;
    typedef vtrc::unique_ptr<protocol_layer_iface> protocol_layer_uptr;
    typedef vtrc::function<protocol_layer_iface *( )> protocol_factory_type;

    namespace dumb {
        protocol_layer_iface *create( ); /// do nothing
    }

    }
}}

#endif // VTRC_CONNECTION_SETUP_IFACE_H
