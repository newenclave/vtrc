
#include <map>
#include "google/protobuf/descriptor.h"

#include "vtrc-rpc-lowlevel.pb.h"
#include "vtrc-rpc-options.pb.h"

#include "vtrc-rpc-channel.h"
#include "vtrc-protocol-layer.h"
#include "vtrc-exception.h"

namespace vtrc { namespace common  {

    namespace gpb = google::protobuf;

    namespace {

    }

    rpc_channel::rpc_channel(unsigned direct_call_type, unsigned callback_type)
        :direct_call_type_(direct_call_type)
        ,callback_type_(callback_type)
    { }

    rpc_channel::~rpc_channel( )
    { }

    rpc_channel::lowlevel_unit_sptr rpc_channel::create_lowlevel(
                                        const gpb::MethodDescriptor *method,
                                        const gpb::Message *request,
                                              gpb::Message *response)
    {
        lowlevel_unit_sptr llu(vtrc::make_shared<lowlevel_unit_type>( ));

        const std::string &serv_name(method->service( )->full_name( ));
        const std::string &meth_name(method->name( ));

        llu->mutable_call( )->set_service_id( serv_name );
        llu->mutable_call( )->set_method_id( meth_name );

        if( request ) {
            llu->set_request( request->SerializeAsString( ) );
        }

        if( !response ) {
            llu->mutable_opt( )->set_accept_response( false );
        }

        return llu;
    }

    rpc_channel::lowlevel_unit_sptr rpc_channel::make_lowlevel(
                        const google::protobuf::MethodDescriptor* method,
                        const google::protobuf::Message* request,
                              google::protobuf::Message* response )
    {
        rpc_channel::lowlevel_unit_sptr res =
                create_lowlevel( method, request, response );
        return res;
    }

    rpc_channel::lowlevel_unit_sptr rpc_channel::raw_call(
                                rpc_channel::lowlevel_unit_sptr /*llu*/,
                                lowlevel_closure_type /*callbacks*/ )
    {
        ;;; /// nothing to do here
        return rpc_channel::lowlevel_unit_sptr( );
    }

    bool can_accept_callbacks( const common::call_context *cc )
    {
#if 1   /// yeap. we always set it up
        rpc::lowlevel_unit const *llu  = cc->get_lowlevel_message( );
        return llu->opt( ).wait( ) && llu->opt( ).accept_callbacks( );
#else
        rpc::options       const *opts = cc->get_call_options( );
        rpc::lowlevel_unit const *llu  = cc->get_lowlevel_message( );

        bool accept_callbacks = llu->opt( ).has_accept_callbacks( )
                                ? llu->opt( ).accept_callbacks( )
                                : opts->accept_callbacks( );

        bool wait = llu->opt( ).has_wait( )
                    ? llu->opt( ).wait( )
                    : opts->wait( );

        return  wait && accept_callbacks;
#endif
    }

    void rpc_channel::configure_message( common::connection_iface_sptr c,
                                         unsigned mess_type,
                                         lowlevel_unit_type &llu ) const
    {
        const common::call_context *cc(common::call_context::get(c));

        llu.set_id(c->get_protocol( ).next_index( ));

        if( mess_type == callback_type_ ) {

            if( cc && can_accept_callbacks( cc ) ) {

                llu.mutable_info( )->set_message_type( mess_type );
                llu.set_target_id( cc->get_lowlevel_message( )->id( ) );

            } else {

                llu.mutable_info( )->set_message_type( direct_call_type_ );

            }
        } else {
            llu.mutable_info( )->set_message_type( mess_type );
        }
    }

    rpc_channel::lowlevel_unit_sptr rpc_channel::call_and_wait_raw (
                        google::protobuf::uint64 call_id,
                        lowlevel_unit_type &llu,
                        common::connection_iface_sptr &cl,
                        lowlevel_closure_type events,
                        const rpc::options *call_opt ) const
    {
        cl->get_protocol( ).call_rpc_method( call_id, llu );

        const unsigned mess_type( llu.info( ).message_type( ) );

        bool wait = true;

        lowlevel_unit_sptr top (vtrc::make_shared<lowlevel_unit_type>( ));

        if( !events ) {
            llu.mutable_opt( )->set_accept_callbacks( false );
        }

        while( wait ) {

            top->Clear( );

            cl->get_protocol( ).read_slot_for( call_id, top,
                                               call_opt->timeout( ) );

            if( top->error( ).code( ) != rpc::errors::ERR_NO_ERROR ) {
                wait = false;
            } else if( top->info( ).message_type( ) != mess_type ) {
                if( events ) {
                    events( *top );
                }
            } else {
                wait = false;
            }
        }
        cl->get_protocol( ).erase_slot( call_id );
        return top;
    }

    void rpc_channel::call_and_wait(
                            google::protobuf::uint64 call_id,
                            const lowlevel_unit_type &llu,
                            google::protobuf::Message *response,
                            connection_iface_sptr &cl,
                            const rpc::options *call_opt ) const
    {
        cl->get_protocol( ).call_rpc_method( call_id, llu );

        const unsigned mess_type(llu.info( ).message_type( ));

        bool wait = true;

        lowlevel_unit_sptr top (vtrc::make_shared<lowlevel_unit_type>( ));

        while( wait ) {

            top->Clear( );

            cl->get_protocol( ).read_slot_for( call_id, top,
                                               call_opt->timeout( ) );

            if( top->error( ).code( ) != rpc::errors::ERR_NO_ERROR ) {
                cl->get_protocol( ).erase_slot( call_id );
                throw vtrc::common::exception( top->error( ).code( ),
                                         top->error( ).category( ),
                                         top->error( ).additional( ) );
            }

            /// from client: call, insertion_call
            /// from server: event, callback
            if( top->info( ).message_type( ) != mess_type ) {
                cl->get_protocol( ).make_local_call( top );
            } else {
                if( response ) {
                    response->ParseFromString( top->response( ) );
                }
                wait = false;
            }
        }
        cl->get_protocol( ).erase_slot( call_id );
    }

    protocol_layer &rpc_channel::get_protocol(connection_iface &cl)
    {
        return cl.get_protocol( );
    }

    void rpc_channel::CallMethod( const gpb::MethodDescriptor *method,
                                        gpb::RpcController *controller,
                                  const gpb::Message *request,
                                        gpb::Message *response,
                                        gpb::Closure *done)
    {
        lowlevel_unit_sptr llu( create_lowlevel( method, request, response ) );
        send_message( *llu, method, controller, request, response, done );
    }

}}

