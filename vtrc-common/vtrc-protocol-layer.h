#ifndef VTRC_PROTOCOL_LAYER_H
#define VTRC_PROTOCOL_LAYER_H

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <deque>
#include <boost/shared_ptr.hpp>

namespace google { namespace protobuf {
    class Message;
    class RpcController;
    class Closure;
    class MethodDescriptor;
}}

namespace vtrc_rpc_lowlevel {
    class lowlevel_unit;
}

namespace vtrc { namespace common {

    namespace data_queue {
        class queue_base;
    }

    struct connection_iface;
    struct hasher_iface;
    struct transformer_iface;
    struct transport_iface;

    class protocol_layer {

        struct impl;
        impl  *impl_;

        friend struct connection_iface;

        protocol_layer( const protocol_layer& other );
        protocol_layer &operator = ( const protocol_layer& other );

    public:

        protocol_layer( transport_iface *connection );
        virtual ~protocol_layer( );

    public:

        virtual bool ready( ) const     = 0;
        uint64_t next_index( );

        void process_data( const char *data, size_t length );
        std::string prepare_data( const char *data, size_t length );

        void send_message( const google::protobuf::Message &message );

        void call_rpc_method( const vtrc_rpc_lowlevel::lowlevel_unit &llu );
        void call_rpc_method( uint64_t slot_id,
                              const vtrc_rpc_lowlevel::lowlevel_unit &llu );

        void wait_call_slot( uint64_t slot_id, uint32_t millisec );
        void wait_call_slot( uint64_t slot_id,
                             std::deque<
                             boost::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit>
                             > &data_list,
                             uint32_t millisec );

        void close_slot( uint64_t slot_id );

    protected:

        void push_rpc_message( uint64_t slot_id,
                  boost::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit> mess);

        virtual void init( )            = 0;
        virtual void on_data_ready( )   = 0;

        bool check_message( const std::string &mess );
        void parse_message( const std::string &mess,
                            google::protobuf::Message &result );

        void pop_message( );

        /* ==== delete this part ==== */

        data_queue::queue_base &get_data_queue( );
        const data_queue::queue_base &get_data_queue( ) const;

        /* ==== delete this part ==== */

        void set_hasher_transformer( hasher_iface *new_hasher,
                                     transformer_iface *new_transformer );

    };
}}

#endif // VTRCPROTOCOLLAYER_H
