#ifndef VTRC_PROTOCOL_LAYER_H
#define VTRC_PROTOCOL_LAYER_H

#include <stdlib.h>

namespace vtrc { namespace common {

    namespace data_queue {
        struct queue_base;
    }

    struct hasher_iface;
    struct transformer_iface;
    struct transport_iface;

    class protocol_layer {

        struct protocol_layer_impl;
        protocol_layer_impl *impl_;

        protocol_layer( const protocol_layer& other );
        protocol_layer &operator = ( const protocol_layer& other );

    public:

        protocol_layer( transport_iface *connection );
        virtual ~protocol_layer( );

    public:

        virtual void init( )            = 0;
        virtual bool ready( ) const     = 0;

        void process_data( const char *data, size_t length );
        void send_data( const char *data, size_t length );

    protected:

        virtual void on_data_ready( )   = 0;

        /* ==== delete this part ==== */

        hasher_iface &get_hasher( );
        const hasher_iface &get_hasher( ) const;

        data_queue::queue_base &get_data_queue( );
        const data_queue::queue_base &get_data_queue( ) const;

        transformer_iface &get_transformer( );
        const transformer_iface &get_transformer( ) const;

        /* ==== delete this part ==== */

        void set_hasher_transformer( hasher_iface *new_hasher,
                                   transformer_iface *new_transformer );

    };
}}

#endif // VTRCPROTOCOLLAYER_H
