#include "google/protobuf/descriptor.h"

#include "vtrc-call-keeper.h"
#include "vtrc-call-context.h"
#include "vtrc-protocol-layer.h"


namespace vtrc { namespace common {

    struct call_keeper::impl {

        connection_iface_wptr       connection_;
        protocol_layer             *protocol_;
        google::protobuf::Closure  *done_;

        protocol_layer::call_stack_type stack_;

        impl( connection_iface_wptr conn, protocol_layer *proto )
            :connection_(conn)
            ,protocol_(proto)
        {
            if(!protocol_->get_call_context( )) {
                throw std::runtime_error( "Call context is not found." );
            }
            protocol_->copy_call_stack( stack_ );
            done_ = stack_.front( )->get_done_closure( );
        }

        ~impl( ) try
        {
            if( done_ ) done_->Run( );
        } catch( ... ) { ;;; }

        bool valid( ) const
        {
            return !!connection_.lock( );
        }

    };

    call_keeper::call_keeper( connection_iface *connection )
        :impl_(new impl(connection->weak_from_this( ),
                        &connection->get_protocol( )))
    { }

    call_keeper::~call_keeper( )
    {
        delete impl_;
    }

    vtrc::shared_ptr<call_keeper> call_keeper::create(connection_iface *conn)
    {
        return vtrc::make_shared<call_keeper>( conn );
    }

    bool call_keeper::valid( ) const
    {
        return impl_->valid( );
    }

}}
