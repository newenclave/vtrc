
#include <map>
#include <google/protobuf/descriptor.h>

#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "vtrc-rpc-channel.h"

namespace vtrc { namespace common  {

    namespace gpb = google::protobuf;


    struct rpc_channel::impl {
    };

    rpc_channel::rpc_channel( )
        :impl_(new impl)
    {}

    rpc_channel::~rpc_channel( )
    {
        delete impl_;
    }

    rpc_channel::lowlevel_unit_sptr rpc_channel::create_lowlevel(
            const gpb::MethodDescriptor *method,
            const gpb::Message *request,
                  gpb::Message *response) const
    {
        lowlevel_unit_sptr llu(vtrc::make_shared<lowlevel_unit_type>( ));

        const std::string &serv_name(method->service( )->full_name( ));
        const std::string &meth_name(method->name( ));

        llu->mutable_call( )->set_service( serv_name );
        llu->mutable_call( )->set_method( meth_name );
        llu->set_request( request->SerializeAsString( ) );
        llu->set_response( response->SerializeAsString( ) );

        return llu;
    }

}}

