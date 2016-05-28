#ifndef VTRC_PROTOCOL_IFACE_H
#define VTRC_PROTOCOL_IFACE_H

#include "vtrc-lowlevel-protocol-iface.h"
#include "vtrc-rpc-channel.h"
#include "vtrc-memory.h"
#include "vtrc-stdint.h"

namespace vtrc { namespace rpc {
    class lowlevel_unit;
    class options;
    class session_options;
}}

namespace google { namespace protobuf {
    class MethodDescriptor;
}}

namespace vtrc { namespace common {

    struct protocol_iface {

        typedef rpc::lowlevel_unit                   lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        virtual ~protocol_iface( ) { }

        virtual vtrc::uint64_t next_index( ) = 0;
        virtual void process_data( const char *data, size_t length ) = 0;

        virtual lowlevel::protocol_layer_iface *get_lowlevel( ) = 0;
        virtual const lowlevel::protocol_layer_iface *get_lowlevel( ) const = 0;

        virtual void drop_service( const std::string &name ) = 0;
        virtual void drop_all_services(  ) = 0;

        virtual const rpc::options *get_method_options(
                        const google::protobuf::MethodDescriptor* method ) = 0;

    //protected: /// need for for rpc_channel

        /// calls for rpc_channel
        friend class rpc_channel;
        virtual void call_rpc_method( const lowlevel_unit_type &llu ) = 0;
        virtual void call_rpc_method( vtrc::uint64_t slot_id,
                                      const lowlevel_unit_type &llu ) = 0;
        virtual void read_slot_for( vtrc::uint64_t slot_id,
                                    lowlevel_unit_sptr &mess,
                                    vtrc::uint64_t microsec ) = 0;
        virtual void erase_slot( vtrc::uint64_t slot_id ) = 0;
        virtual void make_local_call( lowlevel_unit_sptr llu ) = 0;


    };

}}

#endif // VTRCPROTOCOLLAYERIFACE_H

