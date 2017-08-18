#include "google/protobuf/descriptor.h"

#include <stdexcept>

#include "vtrc/common/protocol-layer.h"

#include "vtrc/common/call-keeper.h"
#include "vtrc/common/call-context.h"
#include "vtrc/common/exception.h"

namespace vtrc { namespace common {

    struct call_keeper::impl {

        connection_iface_wptr       connection_;
        google::protobuf::Closure  *done_;

        protocol_layer::call_stack_type stack_;

        impl( connection_iface_wptr conn )
            :connection_(conn)
        {
            if( !protocol_layer::get_call_context( ) ) {
                raise( std::runtime_error( "Call context is not found." ) );
            }
            protocol_layer::copy_call_stack( stack_ );
            done_ = stack_.front( )->get_done_closure( );
        }

        ~impl( ) { try
        {
            if( done_ ) done_->Run( );
        } catch( ... ) { ;;; } }

        bool valid( ) const
        {
            return !!connection_.lock( );
        }

        const call_context *get_call_context( ) const
        {
            return stack_.empty( )
                    ? VTRC_NULL
                    : stack_.front( ).get( );
        }


    };

    call_keeper::call_keeper( connection_iface *connection )
        :impl_(new impl(connection->weak_from_this( )))
    { }

    call_keeper::~call_keeper( )
    {
        delete impl_;
    }

    vtrc::shared_ptr<call_keeper> call_keeper::create(connection_iface *conn)
    {
        return vtrc::make_shared<call_keeper>( conn );
    }

    const call_context *call_keeper::get_call_context( ) const
    {
        return impl_->get_call_context( );
    }

    bool call_keeper::valid( ) const
    {
        return impl_->valid( );
    }

}}

