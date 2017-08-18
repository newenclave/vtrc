#ifdef _WIN32
#include <algorithm> // std::min std::max
#endif

#include "vtrc/common/defaults.h"

#include "vtrc/common/protocol/vtrc-rpc-options.pb.h"
#include "vtrc/common/protocol/vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace common { namespace defaults {

    vtrc::rpc::session_options session_options( )
    {
        rpc::session_options res;
        res.set_max_active_calls  ( 5 );
        res.set_max_message_length( 65536 );
        res.set_max_stack_size    ( 16 );
        res.set_read_buffer_size  ( 4096 );
        res.set_init_timeout      ( 10000 ); // milliseconds
        return res;
    }

    vtrc::rpc::options method_options( )
    {
        vtrc::rpc::options res;
        res.set_timeout( 30000000 ); // microseconds
        res.set_wait( true );
        res.set_accept_callbacks( true );
        return res;
    }

    vtrc::rpc::unit_options unit_options( )
    {
        vtrc::rpc::unit_options res;
        res.set_wait( true );
        res.set_accept_callbacks( true );
        res.set_accept_response( true );
        return res;
    }

}}}
