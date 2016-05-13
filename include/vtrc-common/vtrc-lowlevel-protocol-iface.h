#ifndef VTRC_lowlevel_protocol_layer_IFACE_H
#define VTRC_lowlevel_protocol_layer_IFACE_H

#include <string>
#include "vtrc-closure.h"
#include "vtrc-memory.h"
#include "vtrc-function.h"

namespace google { namespace protobuf {
    class MessageLite;
    class Message;
}}

namespace vtrc { namespace rpc {
    class session_options;
}}

namespace vtrc { namespace common {

    struct protocol_accessor;

    namespace lowlevel {

    struct protocol_layer_iface {
        virtual ~protocol_layer_iface( ) { }

        /// init current protocol
        virtual void init( protocol_accessor *pa,
                           system_closure_type ready_cb ) = 0;

        virtual void close( ) = 0;

        /// do handshake with other side
        virtual void do_handshake( ) = 0;

        /// check for readiness
        virtual bool ready( ) const = 0;

        //// configure current session
        virtual void configure( const rpc::session_options &opts ) = 0;

        /// serialize lowlevel message
        virtual std::string serialize_message(
                            const google::protobuf::Message &mess ) = 0;

        /// pack lowlevel message to protocol view
        virtual std::string pack_message( const char *data, size_t length ) = 0;

        /// accept portion os the data and unpuck message is exists
        virtual void process_data( const char *data, size_t length ) = 0;

        /// parse raw data
        virtual bool parse_raw_message( const std::string &mess,
                                google::protobuf::Message &out ) = 0;

        /// ready messages count
        virtual size_t queue_size( ) const = 0;

        /// validate lowlevel message
        virtual bool check_message( const std::string &mess ) = 0;

        /// get ant pop next message
        virtual bool pop_proto_message(
                             google::protobuf::Message &result ) = 0;

        /// get and pop raw message data
        virtual bool pop_raw_message( std::string &result ) = 0;

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
