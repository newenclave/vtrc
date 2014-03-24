
#include <google/protobuf/descriptor.h>

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common-channel.h"

#include "protocol/vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace server {

    namespace gpb = google::protobuf;

    namespace {

        typedef vtrc_rpc_lowlevel::lowlevel_unit lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

    }

    void common_channel::CallMethod(const gpb::MethodDescriptor* method,
                    gpb::RpcController* controller,
                    const gpb::Message* request,
                    gpb::Message* response,
                    gpb::Closure* done)
    {
        lowlevel_unit_sptr llu( create_lowlevel( method, request, response ) );

        send_message( llu, method, controller, done );
    }

    common_channel::common_channel( )
    { }

    common_channel::~common_channel( )
    { }

}}

