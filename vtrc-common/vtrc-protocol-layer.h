#ifndef VTRC_PROTOCOL_LAYER_H
#define VTRC_PROTOCOL_LAYER_H

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <deque>
#include "vtrc-memory.h"

#include "vtrc-call-context.h"
#include "vtrc-rpc-service-wrapper.h"

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

namespace boost { namespace system {
    class error_code;
}}

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

        struct        impl;
        friend struct impl;
        impl         *impl_;

        friend struct connection_iface;

        protocol_layer( const protocol_layer& other );
        protocol_layer &operator = ( const protocol_layer& other );

    public:

        typedef vtrc_rpc_lowlevel::lowlevel_unit     lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        typedef std::deque<std::string> message_queue_type;

        protocol_layer( transport_iface *connection, bool oddside );
        virtual ~protocol_layer( );

    public:

        virtual bool ready( ) const     = 0;
        uint64_t next_index( );

        void process_data( const char *data, size_t length );
        std::string prepare_data( const char *data, size_t length );

        virtual void on_write_error( const boost::system::error_code &err );
        virtual void on_read_error( const boost::system::error_code &err );

        //void send_message( const google::protobuf::Message &message );

        void make_call( lowlevel_unit_sptr llu );
        void make_call(protocol_layer::lowlevel_unit_sptr llu,
                       common::closure_type done);

        void call_rpc_method( const vtrc_rpc_lowlevel::lowlevel_unit &llu );
        void call_rpc_method( uint64_t slot_id,
                              const vtrc_rpc_lowlevel::lowlevel_unit &llu );

        // refactor names here!
        void wait_slot_for( uint64_t slot_id, uint32_t millisec );

        void read_slot_for( uint64_t slot_id,
                            lowlevel_unit_sptr &mess,
                            uint32_t millisec );

        void read_slot_for( uint64_t slot_id,
                            std::deque<lowlevel_unit_sptr> &mess_list,
                            uint32_t millisec );

        void erase_slot ( uint64_t slot_id );
        void cancel_slot( uint64_t slot_id );

        void erase_all_slots( );
        void cancel_all_slots( );
        //void close_queue( );

        const vtrc_rpc_lowlevel::options &get_method_options(
                            const google::protobuf::MethodDescriptor* method );

    protected:

        friend class rpc_channel;

        struct context_holder {
            protocol_layer *p_;
            call_context   *ctx_;
            context_holder( protocol_layer *parent,
                            lowlevel_unit_type *llu )
                :p_(parent)

            {
                ctx_ = new call_context( llu );
                p_->push_call_context( ctx_ );
            }

            ~context_holder( ) try
            {
                p_->pop_call_context( );
            } catch( ... ) { ;;; /*Cthulhu*/ }

        private:
            context_holder( context_holder const & );
            context_holder &operator = ( context_holder const & );
        };

        friend struct context_holder;

    protected:

        virtual void init( )            = 0;
        virtual void on_data_ready( )   = 0;

        typedef std::deque< vtrc::shared_ptr<call_context> > call_stack_type;

        call_context *push_call_context ( call_context *cc );
        call_context *push_call_context ( vtrc::shared_ptr<call_context> cc );
        void           pop_call_context ( );
        call_context  *top_call_context ( );
        void         reset_call_stack   ( );
        void          swap_call_stack   ( call_stack_type& other );
        void          copy_call_stack   ( call_stack_type& other ) const;

    public:

        const call_context *get_call_context( ) const;

    protected:

        void push_rpc_message( uint64_t slot_id, lowlevel_unit_sptr mess);

        void push_rpc_message_all( lowlevel_unit_sptr mess );

        virtual rpc_service_wrapper_sptr get_service_by_name(
                                                const std::string &name ) = 0;

        bool check_message( const std::string &mess );
        void parse_message( const std::string &mess,
                            google::protobuf::Message &result );

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
