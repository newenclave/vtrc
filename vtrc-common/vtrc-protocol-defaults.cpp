#include "vtrc-common/vtrc-protocol-defaults.h"

#include "protocol/vtrc-rpc-options.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace common { namespace defaults {

    vtrc::rpc::session_options session_options( )
    {
        rpc::session_options res;
        res.set_max_active_calls  ( 5 );
        res.set_max_message_length( 65536 );
        res.set_max_stack_size    ( 64 );
        res.set_read_buffer_size  ( 4096 );
        return res;
    }

    vtrc::rpc::options method_options( )
    {
        rpc::options res;
        res.set_timeout( 30000000 ); // microseconds
        res.set_wait( true );
        res.set_accept_callbacks( true );
        return res;
    }

    vtrc::rpc::unit_options unit_options( )
    {
        rpc::unit_options res;
        res.set_wait( true );
        res.set_accept_callbacks( true );
        res.set_accept_response( true );
        return res;
    }

}}}
