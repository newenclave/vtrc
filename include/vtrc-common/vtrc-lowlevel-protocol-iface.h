#ifndef VTRC_lowlevel_protocol_layer_IFACE_H
#define VTRC_lowlevel_protocol_layer_IFACE_H

#include <string>
#include "vtrc-closure.h"
#include "vtrc-memory.h"

namespace google { namespace protobuf {
    class MessageLite;
    class Message;
}}

namespace vtrc { namespace rpc {
    class session_options;
}}

namespace vtrc { namespace common {

    struct protocol_accessor;

    struct lowlevel_protocol_layer_iface {
        virtual ~lowlevel_protocol_layer_iface( ) { }

        /// init current protocol
        virtual void init( protocol_accessor *pa,
                           system_closure_type ready_cb ) = 0;

        /// do handshake with other side
        virtual bool do_handshake( const std::string &data ) = 0;

        /// check for readiness
        virtual bool ready( ) const = 0;

        //// configure current session
        virtual void configure( const rpc::session_options &opts ) = 0;

        /// pack lowlevel message to protocol view
        virtual std::string process_message( const char *data,
                                             size_t length ) = 0;

        /// accept portion os the data and unpuck message is exists
        virtual void process_data( const char *data, size_t length ) = 0;

        /// ready messages count
        virtual size_t queue_size( ) const = 0;

        /// validate lowlevel message
        virtual bool check_message( const std::string &mess ) = 0;

        /// get ant pop next message
        virtual bool pop_proto_message(
                             google::protobuf::MessageLite &result ) = 0;

        /// get and pop raw message data
        virtual bool pop_raw_message( std::string &result ) = 0;

        /// parse raw data
        virtual bool parse_raw_message( const std::string &mess,
                                google::protobuf::MessageLite &out ) = 0;

    };

    typedef vtrc::shared_ptr<
        lowlevel_protocol_layer_iface
    > lowlevel_protocol_layer_sptr;

    typedef vtrc::unique_ptr<
        lowlevel_protocol_layer_iface
    > lowlevel_protocol_layer_uptr;

}}

#endif // VTRC_CONNECTION_SETUP_IFACE_H
