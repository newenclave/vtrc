#ifndef VTRC_PROTOCOL_LAYER_H
#define VTRC_PROTOCOL_LAYER_H

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <deque>
#include "vtrc-memory.h"

namespace google { namespace protobuf {
    class Message;
    class RpcController;
    class Closure;
    class MethodDescriptor;
}}

namespace vtrc_rpc_lowlevel {
    class lowlevel_unit;
    class options;
}

namespace vtrc { namespace common {

    namespace data_queue {
        class queue_base;
    }

    struct connection_iface;
    struct hash_iface;
    struct transformer_iface;
    struct transport_iface;
    class  call_context;

    class protocol_layer {

        struct impl;
        impl  *impl_;

        friend struct connection_iface;

        protocol_layer( const protocol_layer& other );
        protocol_layer &operator = ( const protocol_layer& other );

    public:

        typedef std::deque<std::string> message_queue_type;

        protocol_layer( transport_iface *connection );
        virtual ~protocol_layer( );

    public:

        virtual bool ready( ) const     = 0;
        uint64_t next_index( );

        void process_data( const char *data, size_t length );
        std::string prepare_data( const char *data, size_t length );

        void send_message( const google::protobuf::Message &message );

        const call_context *get_call_context( ) const;

        void call_rpc_method( const vtrc_rpc_lowlevel::lowlevel_unit &llu );
        void call_rpc_method( uint64_t slot_id,
                              const vtrc_rpc_lowlevel::lowlevel_unit &llu );

        void wait_call_slot( uint64_t slot_id, uint32_t millisec );
        void wait_call_slot( uint64_t slot_id,
                           std::deque<
                              vtrc::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit>
                           > &data_list,
                           uint32_t millisec );

        void close_slot( uint64_t slot_id );

        const vtrc_rpc_lowlevel::options &get_method_options(
                            const google::protobuf::MethodDescriptor* method );

    protected:

        struct context_holder {
            protocol_layer *p_;
            call_context   *ctx_;
            context_holder( protocol_layer *parent,
                            vtrc_rpc_lowlevel::lowlevel_unit *llu )
                :p_(parent)
                ,ctx_(p_->create_call_context( llu ))
            { }

            ~context_holder( )
            {
                p_->clear_call_context( );
            }
        private:
            context_holder( context_holder const & );
            context_holder &operator = ( context_holder const & );
        };

        void push_rpc_message( uint64_t slot_id,
                    vtrc::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit> mess);

        virtual void init( )            = 0;
        virtual void on_data_ready( )   = 0;

        bool check_message( const std::string &mess );
        void parse_message( const std::string &mess,
                            google::protobuf::Message &result );

        call_context *create_call_context (
                                        vtrc_rpc_lowlevel::lowlevel_unit *llu );

        void clear_call_context( );

        void pop_message( );

        /* ==== delete this part ==== */

        message_queue_type &message_queue( );
        const message_queue_type &message_queue( ) const;

        /* ==== delete this part ==== */

        void change_hash_maker( hash_iface *new_hasher );
        void change_hash_checker( hash_iface *new_hasher );

        void change_transformer( transformer_iface *new_transformer);
        void change_reverter( transformer_iface *new_reverter);

    };
}}

#endif // VTRCPROTOCOLLAYER_H
