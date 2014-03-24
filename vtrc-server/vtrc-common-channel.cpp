
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

//    struct common_channel::impl {

//    };

    void common_channel::CallMethod(const gpb::MethodDescriptor* method,
                    gpb::RpcController* controller,
                    const gpb::Message* request,
                    gpb::Message* response,
                    gpb::Closure* done)
    {
        lowlevel_unit_sptr llu( vtrc::make_shared<lowlevel_unit_type>( ) );
        const std::string &serv_name(method->service( )->full_name( ));
        const std::string &meth_name(method->full_name( ));

        llu->mutable_call( )->set_service( serv_name );
        llu->mutable_call( )->set_method( meth_name );
        llu->set_request( request->SerializeAsString( ) );
        llu->set_response( response->SerializeAsString( ) );

        send_message( llu, method, controller, done );
    }

    common_channel::common_channel( )
        //:impl_(new impl)
    { }

    common_channel::~common_channel( )
    {
        //delete impl_;
    }

}}

