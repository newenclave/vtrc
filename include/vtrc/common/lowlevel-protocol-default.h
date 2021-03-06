#ifndef VTRC_DEFAULT_LOWLEVEL_PROTOCOL_H
#define VTRC_DEFAULT_LOWLEVEL_PROTOCOL_H

#include "vtrc/common/lowlevel-protocol-iface.h"
#include "vtrc/common/hash/iface.h"
#include "vtrc/common/transformer/iface.h"
#include "vtrc/common/data-queue.h"

#include "vtrc-memory.h"

namespace vtrc {  namespace common { namespace lowlevel {

    class default_protocol: public protocol_layer_iface {

        hash::iface::uptr                        hash_maker_;
        hash::iface::uptr                        hash_checker_;
        transformer::iface::uptr                 transformer_;
        transformer::iface::uptr                 revertor_;
        vtrc::unique_ptr<data_queue::queue_base> queue_;
        vtrc::function<void(void)>               process_stage_;

        protocol_accessor *pa_;

    public:

        virtual void init( protocol_accessor *pa,
                           system_closure_type ready_cb );
        virtual void close( );
        virtual void do_handshake( );

        void set_transformer( transformer::iface *ti );
        void set_revertor( transformer::iface *ti );
        void set_hash_maker( hash::iface *hi );
        void set_hash_checker( hash::iface *hi );

        default_protocol( );
        ~default_protocol( );

        void        configure( const rpc::session_options &opts );
        std::string serialize_lowlevel( const vtrc::rpc::lowlevel_unit &mess );
        void        pack_message( std::string &mess );
        void        process_data( const char *data, size_t length );

        size_t      queue_size( ) const;
        bool        check_message( const std::string &mess );
        bool        pop_proto_message(rpc::lowlevel_unit &result );

        bool        pop_raw_message( std::string &result );
        bool        parse_raw_message( const std::string &mess,
                                       google::protobuf::Message &out );

    protected:

        void ready_call( );
        void switch_to_ready( );
        void switch_to_handshake( );

        std::string pack_message_impl( const char *data, size_t length );
        std::string pack_message_impl( const std::string &data );

        virtual void configure_impl( const rpc::session_options &opts );
        void set_accessor( protocol_accessor *pa );
        protocol_accessor *accessor( );

    };
}}}

#endif // VTRCDEFAULTLOWLEVELPROTOCOL_H

