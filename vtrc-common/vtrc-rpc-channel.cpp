
#include <map>
#include <google/protobuf/descriptor.h>

#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "vtrc-rpc-channel.h"
#include "vtrc-protocol-layer.h"
#include "vtrc-exception.h"
#include "vtrc-chrono.h"

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

    void rpc_channel::process_waitable_call(google::protobuf::uint64 call_id,
                rpc_channel::lowlevel_unit_sptr &llu,
                google::protobuf::Message *response,
                connection_iface_sptr &cl,
                const vtrc_rpc_lowlevel::options &call_opt) const
    {
        cl->get_protocol( ).call_rpc_method( call_id, *llu );

        const unsigned mess_type(llu->info( ).message_type( ));

        bool wait = true;

        while( wait ) {

            lowlevel_unit_sptr top
                    (vtrc::make_shared<lowlevel_unit_type>( ));

            cl->get_protocol( ).read_slot_for( call_id, top,
                                               call_opt.call_timeout( ) );

            if( top->error( ).code( ) != vtrc_errors::ERR_NO_ERROR ) {
                cl->get_protocol( ).erase_slot( call_id );
                throw vtrc::common::exception( top->error( ).code( ),
                                         top->error( ).category( ),
                                         top->error( ).additional( ) );
            }

            // client: call. server: event, callback
            if( top->info( ).message_type( ) != mess_type ) {
                cl->get_protocol( ).make_call( top );
            } else {
                response->ParseFromString( top->response( ) );
                wait = false;
            }
        }
        cl->get_protocol( ).erase_slot( call_id );
    }

}}

