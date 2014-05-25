#ifndef STRESS_IFACE_H
#define STRESS_IFACE_H

#include "vtrc-memory.h"

#include "vtrc-common/vtrc-rpc-channel.h"

namespace vtrc { namespace client {
    class vtrc_client;
}}

namespace stress {

    struct interface {

        virtual ~interface( ) { }

        virtual void ping( unsigned payload ) = 0;

    };

    interface *create_stress_client(
            vtrc::shared_ptr<vtrc::client::vtrc_client> c,
            vtrc::common::rpc_channel::options opts);

    interface *create_stress_client(
            vtrc::shared_ptr<vtrc::client::vtrc_client> c); /// default options

}

#endif // STRESSIFACE_H