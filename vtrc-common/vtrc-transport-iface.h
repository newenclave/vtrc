#ifndef VTRC_TRANSPORT_IFACE_H
#define VTRC_TRANSPORT_IFACE_H

#include <stdlib.h>

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-protocol-layer.h"

#include "vtrc-asio.h"

namespace boost {

    namespace system {
        class error_code;
    }
}

namespace vtrc { namespace common {


    struct transport_iface: public connection_iface {

        virtual ~transport_iface( ) { }

        virtual std::string name( ) const   = 0;
        virtual void close( )               = 0;
        virtual bool active( ) const        = 0;
        virtual void init( )                = 0;

//        virtual VTRCASIO::io_service::strand &get_dispatcher( ) = 0;

        void raw_write ( vtrc::shared_ptr<rpc::lowlevel_unit> ll_mess )
        {
            std::string ser( ll_mess->SerializeAsString( ) );
            write( ser.empty( ) ? "" : &ser[0], ser.size( ) );
        }

        virtual void write( const char *data, size_t length ) = 0;

        virtual void write( const char *data, size_t length,
                                        const system_closure_type &success,
                                        bool success_on_send ) = 0;
        virtual void on_close( ) { }

        virtual void drop_service( const std::string &name )
        {
            get_protocol( ).drop_service( name );
        }

        virtual void drop_all_services(  )
        {
            get_protocol( ).drop_all_services( );
        }

    };

}}

#endif // VTRC_TRANSPORT_IFACE_H
