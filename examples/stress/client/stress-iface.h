#ifndef STRESS_IFACE_H
#define STRESS_IFACE_H

#include "vtrc-memory.h"

#include "vtrc/common/rpc-channel.h"

namespace vtrc { namespace client {
    class vtrc_client;
}}

namespace stress {

    struct interface {

        virtual ~interface( ) { }

        virtual void ping( unsigned payload ) = 0;
        virtual void generate_events( unsigned count,
                                      bool insert,
                                      bool wait,
                                      unsigned payload ) = 0;

        virtual void recursive_call( unsigned count, unsigned payload ) = 0;
        virtual unsigned register_timer( unsigned microsec ) = 0;

        virtual void shutdown( ) = 0;
        virtual void close_me( ) = 0;
    };

    interface *create_stress_client(
            vtrc::shared_ptr<vtrc::client::vtrc_client> c,
            vtrc::common::rpc_channel::flags opts, bool init = false);

    interface *create_stress_client(
            vtrc::shared_ptr<vtrc::client::vtrc_client> c); /// default options

}

#endif // STRESSIFACE_H
