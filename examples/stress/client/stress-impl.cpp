#include "stress-iface.h"

#include "protocol/stress.pb.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-client/vtrc-client.h"

namespace stress {

using namespace vtrc;

namespace {

    typedef vtrc_example::stress_service::Stub stub_type;
    typedef common::stub_wrapper<stub_type>    stub_wrapper_type;
    namespace gpb = google::protobuf;

    struct impl: public interface {

        typedef common::rpc_channel channel_type;

        stub_wrapper_type stub_;

        vtrc::shared_ptr<channel_type> get_channel( client::vtrc_client &c,
                                                    channel_type::flags opts)
        {
            vtrc::shared_ptr<channel_type> new_ch(c.create_channel( opts ));
            return new_ch;
        }

        impl( vtrc::shared_ptr<client::vtrc_client> &c,
              common::rpc_channel::flags opts )
            :stub_(get_channel(*c, opts))
        {
            stub_.call( &stub_type::init );
        }

        void ping( unsigned payload )
        {
            std::string payload_data( payload, '!' );
            vtrc_example::ping_req req;
            req.mutable_payload( )->swap( payload_data );
            stub_.call_request( &stub_type::ping, &req );
        }

        void generate_events( unsigned count,
                              bool insert, bool wait,
                              unsigned payload )
        {
            vtrc_example::generate_events_req req;
            req.set_count( count );
            req.set_callback( insert );
            req.set_wait( wait );
            req.set_payload_size( payload );
            stub_.call_request( &stub_type::generate_event, &req );
        }

        void recursive_call( unsigned count, unsigned payload )
        {
            vtrc_example::recursive_call_req req;
            req.set_balance( count );
            req.set_payload( std::string( payload, '+' ) );
            stub_.call_request( &stub_type::recursive_call, &req );
        }

        unsigned register_timer( unsigned microsec )
        {
            vtrc_example::timer_register_req req;
            vtrc_example::timer_register_res res;
            req.set_microseconds( microsec );
            stub_.call( &stub_type::reg, &req, &res );
            return res.id( );
        }

        void shutdown( )
        {
            stub_.call( &stub_type::shutdown );
        }

    };
}

    interface *create_stress_client(vtrc::shared_ptr<client::vtrc_client> c,
                                    common::rpc_channel::flags opts)
    {
        return new impl( c, opts );
    }

    interface *create_stress_client(
            vtrc::shared_ptr<vtrc::client::vtrc_client> c) /// default options
    {
        return create_stress_client( c, common::rpc_channel::DEFAULT );
    }

}

