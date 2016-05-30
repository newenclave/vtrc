
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-protocol-layer.h"

namespace vtrc { namespace common {

    namespace ll = lowlevel;

    const ll::protocol_layer_iface *connection_iface::get_lowlevel( ) const
    {
        return get_protocol( ).get_lowlevel( );
    }

    void connection_iface::drop_service( const std::string &name )
    {
        get_protocol( ).drop_service( name );
    }

    void connection_iface::drop_all_services(  )
    {
        get_protocol( ).drop_all_services( );
    }

    void connection_iface::raw_write( vtrc::shared_ptr<rpc::lowlevel_unit> ll )
    {
        std::string ser( get_protocol( ).get_lowlevel( )
                         ->pack_message( *ll ) );
        write( ser.empty( ) ? "" : &ser[0], ser.size( ) );
    }

}}

