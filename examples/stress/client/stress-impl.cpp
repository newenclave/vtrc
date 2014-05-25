#include "stress-iface.h"

#include "protocol/stress.pb.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-client-base/vtrc-client.h"


namespace stress {

using namespace vtrc;

namespace {

    typedef vtrc_example::stress_service::Stub stub_type;
    typedef common::stub_wrapper<stub_type>    stub_wrapper_type;

    struct impl: public interface {

        stub_wrapper_type stub_;

        impl( vtrc::shared_ptr<client::vtrc_client> &c,
              common::rpc_channel::options opts )
            :stub_(c->create_channel(opts))
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

    };
}

    interface *create_stress_client(vtrc::shared_ptr<client::vtrc_client> c,
                                    common::rpc_channel::options opts)
    {
        return new impl( c, opts );
    }

    interface *create_stress_client(
            vtrc::shared_ptr<vtrc::client::vtrc_client> c) /// default options
    {
        return create_stress_client( c, common::rpc_channel::DEFAULT );
    }

}
