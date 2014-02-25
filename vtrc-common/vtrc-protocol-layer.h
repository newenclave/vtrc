#ifndef VTRC_PROTOCOL_LAYER_H
#define VTRC_PROTOCOL_LAYER_H

#include <stdlib.h>
#include <stdint.h>
#include <string>

namespace google { namespace protobuf {
    class Message;
}}

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

    protected:

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
