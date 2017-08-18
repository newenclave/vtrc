#ifndef VTRC_lowlevel_protocol_layer_IFACE_H
#define VTRC_lowlevel_protocol_layer_IFACE_H

#include <string>
#include "vtrc-memory.h"
#include "vtrc-function.h"

#include "vtrc/common/closure.h"
#include "vtrc/common/protocol/accessor-iface.h"

namespace google { namespace protobuf {
    class MessageLite;
    class Message;
    class MethodDescriptor;
}}

namespace vtrc { namespace rpc {
    class session_options;
    class lowlevel_unit;
}}

namespace vtrc { namespace common {

    namespace lowlevel {

    struct protocol_layer_iface {

        typedef google::protobuf::MethodDescriptor  message_descriptor_type;
        typedef google::protobuf::Message           message_type;
        typedef rpc::lowlevel_unit                  lowlevel_unit;

        virtual ~protocol_layer_iface( ) { }

        /// init current protocol
        virtual void init( protocol_accessor *pa,
                           system_closure_type ready_for_read ) = 0;

        /// connection is closing
        virtual void close( ) = 0;

        /// configure session
        virtual void configure( const rpc::session_options & ) { }

        /// serialize lowlevel message; threads!
        virtual std::string serialize_lowlevel( const lowlevel_unit &mess ) = 0;

        /// pack lowlevel message; non threads!
        virtual void pack_message( std::string & /*mess*/ ) { }

        /// accept portion os the data and unpuck message is exists
        virtual void process_data( const char *data, size_t length ) = 0;

        /// ready messages count
        virtual size_t queue_size( ) const = 0;

        /// get and pop next message
        virtual bool pop_proto_message( rpc::lowlevel_unit &result ) = 0;

        /// serialize message request or response
        /// SerializeAsString as default
        virtual std::string serialize_message( const message_type *m );

        /// parse request or response
        /// ParseFromString as default
        virtual void parse_message( const std::string &data, message_type *m );

        virtual std::string service_id( const message_descriptor_type *desc );
        virtual std::string method_id ( const message_descriptor_type *desc );
    };

    typedef vtrc::shared_ptr<protocol_layer_iface> protocol_layer_sptr;
    typedef vtrc::unique_ptr<protocol_layer_iface> protocol_layer_uptr;
    typedef vtrc::function<protocol_layer_iface *( )> protocol_factory_type;

    namespace dummy {
        protocol_layer_iface *create( ); /// do nothing
    }

    }
}}

#endif // VTRC_CONNECTION_SETUP_IFACE_H
