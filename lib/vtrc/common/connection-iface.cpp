
#include "vtrc/common/connection-iface.h"
#include "vtrc/common/protocol-layer.h"

#include <sstream>

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
                         ->serialize_lowlevel( *ll ) );
        write( ser.empty( ) ? "" : &ser[0], ser.size( ) );
    }


    connection_empty::~connection_empty( )
    {
        if( protocol_ ) {
            delete protocol_;
        }
    }

    std::string connection_empty::name( ) const
    {
        std::ostringstream oss;
        oss << "empty://0x" << std::hex << reinterpret_cast<const void *>(this);
        return oss.str( );
    }

}}

