
#include "vtrc-common/vtrc-connection-setup-iface.h"


namespace vtrc { namespace server {

    namespace {

        struct iface: public common::connection_setup_iface {

            void init( protocol_accessor *pa )
            {

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


}}

