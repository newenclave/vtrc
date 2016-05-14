#ifndef VTRC_DEFAULT_LOWLEVEL_PROTOCOL_H
#define VTRC_DEFAULT_LOWLEVEL_PROTOCOL_H

#include "vtrc-common/vtrc-lowlevel-protocol-iface.h"
#include "vtrc-common/vtrc-hash-iface.h"
#include "vtrc-common/vtrc-transformer-iface.h"
#include "vtrc-common/vtrc-data-queue.h"

#include "vtrc-memory.h"

namespace vtrc {  namespace common { namespace lowlevel {

    class default_protocol: public protocol_layer_iface {

        vtrc::unique_ptr<hash_iface>             hash_maker_;
        vtrc::unique_ptr<hash_iface>             hash_checker_;
        vtrc::unique_ptr<transformer_iface>      transformer_;
        vtrc::unique_ptr<transformer_iface>      revertor_;
        vtrc::unique_ptr<data_queue::queue_base> queue_;

        protocol_accessor *pa_;

    public:

        virtual void init( protocol_accessor *pa,
                           system_closure_type ready_cb );
        virtual void close( );
        virtual void do_handshake( );
        virtual bool ready( ) const;

        void set_transformer( transformer_iface *ti );
        void set_revertor( transformer_iface *ti );
        void set_hash_maker( hash_iface *hi );
        void set_hash_checker( hash_iface *hi );

        default_protocol( );
        ~default_protocol( );

        void        configure( const rpc::session_options &opts );
        std::string serialize_message( const google::protobuf::Message &mess );
        std::string pack_message( const char *data, size_t length );
        void        process_data( const char *data, size_t length );
        size_t      queue_size( ) const;
        bool        check_message( const std::string &mess );
        bool        pop_proto_message( google::protobuf::Message &result );
        bool        pop_raw_message( std::string &result );
        bool        parse_raw_message( const std::string &mess,
                                       google::protobuf::Message &out );

    protected:

        virtual void configure_impl( const rpc::session_options &opts );
        void set_accessor( protocol_accessor *pa );
        protocol_accessor *accessor( );

    };
}}}

#endif // VTRCDEFAULTLOWLEVELPROTOCOL_H
