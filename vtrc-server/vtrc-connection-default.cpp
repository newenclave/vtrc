
#include "vtrc-common/vtrc-connection-setup-iface.h"


namespace vtrc { namespace server {

    namespace {

        struct iface: public common::connection_setup_iface {

            common::protocol_accessor *pa_;

            iface( )
            { }

            void init( common::protocol_accessor *pa )
            {
                pa_ = pa;
            }

            bool next( const std::string &data )
            {

            }

            bool error( rpc::errors::container *err )
            {

            }

            bool close_connection( )
            {

            }
        };
    }

    common::connection_setup_iface *create_default_setup( )
    {
        return new iface;
    }

}}

