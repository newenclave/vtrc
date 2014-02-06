#ifndef VTRCP_ROTOCOL_LAYER_H
#define VTRCP_ROTOCOL_LAYER_H

namespace vtrc { namespace server {

    struct connection_iface;

    class protocol_layer {

        struct protocol_layer_impl;
        protocol_layer_impl *impl_;

        protocol_layer( const protocol_layer& other );
        protocol_layer operator = ( const protocol_layer& other );

    public:

        protocol_layer( connection_iface *connection );
        ~protocol_layer( );

    public:

        void init( );
        void process_data( const char *data, size_t length );

    };

}}



#endif // VTRCP_ROTOCOL_LAYER_H
