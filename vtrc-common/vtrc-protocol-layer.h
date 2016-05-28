#ifndef VTRC_PROTOCOL_LAYER_H
#define VTRC_PROTOCOL_LAYER_H

#include <stdlib.h>
#include <string>
#include <deque>

#include "vtrc-memory.h"
#include "vtrc-stdint.h"
#include "vtrc-system-forward.h"

#include "vtrc-common/vtrc-call-context.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-lowlevel-protocol-iface.h"
#include "vtrc-common/vtrc-protocol-iface.h"

VTRC_SYSTEM_FORWARD( class error_code; )

namespace google { namespace protobuf {
    class Message;
    class MessageLite;
    class RpcController;
    class Closure;
    class ServiceDescriptor;
    class MethodDescriptor;
}}

namespace vtrc { namespace rpc {

    class lowlevel_unit;
    class options;
    class session_options;

    namespace errors {
        class container;
    }

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

    class protocol_layer: public protocol_iface {

        struct        impl;
        friend struct impl;
        impl         *impl_;

        friend struct connection_iface;

        protocol_layer( const protocol_layer& other );
        protocol_layer &operator = ( const protocol_layer& other );

    public:

        typedef rpc::lowlevel_unit                   lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        typedef std::deque<std::string> message_queue_type;

        protocol_layer( transport_iface *connection, bool oddside );
        protocol_layer( transport_iface *connection, bool oddside,
                        const rpc::session_options &opts );
        virtual ~protocol_layer( );

    public:

        bool ready( ) const;

        /// ================= ///

        void set_ready( bool ready );
        void set_lowlevel( common::lowlevel::protocol_layer_iface *ll );
        common::lowlevel::protocol_layer_iface *get_lowlevel( );
        const common::lowlevel::protocol_layer_iface *get_lowlevel( ) const;

        /// ================= ///

        vtrc::uint64_t next_index( );

        void process_data( const char *data, size_t length );

        virtual void on_write_error( const VTRC_SYSTEM::error_code &err );
        virtual void on_read_error ( const VTRC_SYSTEM::error_code &err );

        /// ================= ///

        void set_precall( const precall_closure_type &func );
        void set_postcall( const postcall_closure_type &func );

        void make_local_call( lowlevel_unit_sptr llu );
        void make_local_call( lowlevel_unit_sptr llu,
                              const protcol_closure_type &done);

        void call_rpc_method( const lowlevel_unit_type &llu );
        void call_rpc_method( vtrc::uint64_t slot_id,
                              const lowlevel_unit_type &llu );

        void wait_slot_for( vtrc::uint64_t slot_id, vtrc::uint64_t microsec );

        void read_slot_for( vtrc::uint64_t slot_id,
                            lowlevel_unit_sptr &mess,
                            vtrc::uint64_t microsec );

        void read_slot_for( vtrc::uint64_t slot_id,
                            std::deque<lowlevel_unit_sptr> &mess_list,
                            vtrc::uint64_t microsec );

        void erase_slot ( vtrc::uint64_t slot_id );
        void cancel_slot( vtrc::uint64_t slot_id );
        bool slot_exists( vtrc::uint64_t slot_id ) const;

        void erase_all_slots( );
        void cancel_all_slots( );

        const rpc::options *get_method_options(
                            const google::protobuf::MethodDescriptor* method );

//        const rpc::options *get_method_options(
//                            const lowlevel_unit_type &llu );

        const rpc::session_options &session_options( ) const;

//    protected:

        struct context_holder {
            call_context   *ctx_;
            context_holder( lowlevel_unit_type *llu )
            {
                ctx_ = new call_context( llu );
                protocol_layer::push_call_context( ctx_ );
            }

            ~context_holder( )
            {
                try {
                    protocol_layer::pop_call_context( );
                } catch( ... ) { ;;; /*Cthulhu*/ }
            }

        private:
            context_holder( context_holder const & );
            context_holder &operator = ( context_holder const & );
        };

        friend class  rpc_channel;
        friend class  call_keeper;
        friend struct context_holder;

    protected:

        //// pure virtual

        virtual void init( ) = 0;
        virtual const std::string &client_id( ) const = 0;
        virtual rpc_service_wrapper_sptr get_service_by_name(
                                         const std::string &name ) = 0;

        ////

        typedef std::deque< vtrc::shared_ptr<call_context> > call_stack_type;

        /**
         * call context
        **/
        static
        call_context  *push_call_context ( call_context *cc );
        static
        call_context  *push_call_context ( vtrc::shared_ptr<call_context> cc );
        static
        void           pop_call_context  ( );
        static
        call_context  *top_call_context  ( );
        static
        void           reset_call_stack  ( );
        static
        void           swap_call_stack   ( call_stack_type& other );
        static
        void           copy_call_stack   ( call_stack_type& other );

    public:

        static const call_context *get_call_context( );

        virtual void drop_service( const std::string &name ) = 0;
        virtual void drop_all_services(  ) = 0;

    protected:

        void configure_session( const rpc::session_options &opts );

        size_t push_rpc_message( vtrc::uint64_t slot_id,
                                 lowlevel_unit_sptr mess);

        size_t push_rpc_message_all( lowlevel_unit_sptr mess );

        size_t ready_messages_count( ) const;
        bool   message_queue_empty( ) const;

        /// false == bad message;
        /// protocol violation; we have to close connection in this case
        bool   parse_and_pop( rpc::lowlevel_unit &result );

    };
}}

#endif // VTRCPROTOCOLLAYER_H
