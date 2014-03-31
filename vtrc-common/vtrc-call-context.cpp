#include <stdlib.h>
#include "vtrc-call-context.h"
#include "vtrc-connection-iface.h"
#include "vtrc-protocol-layer.h"

namespace vtrc { namespace common {

    typedef vtrc_rpc_lowlevel::lowlevel_unit lowlevel_unit;

    struct call_context::impl {
        lowlevel_unit                    *llu_;
        call_context                     *parent_context_;
        const vtrc_rpc_lowlevel::options *opts_;
        google::protobuf::Closure        *closure_;
        impl(lowlevel_unit *llu)
            :llu_(llu)
            ,opts_(NULL)
            ,closure_(NULL)
        { }
    };

    call_context::call_context( lowlevel_unit *lowlevel )
        :impl_(new impl(lowlevel))
    { }

    const call_context *call_context::get( connection_iface *iface )
    {
        return iface->get_protocol( ).get_call_context( );
    }

    const call_context *call_context::get(connection_iface_sptr iface)
    {
        return get( iface.get( ) );
    }

    call_context::call_context( const call_context &other )
        :impl_(new impl(*other.impl_))
    { }

    call_context &call_context::operator = ( const call_context &other )
    {
        *impl_ = *other.impl_;
        return   *this;
    }

    call_context *call_context::next( )
    {
        return impl_->parent_context_;
    }

    const call_context *call_context::next( ) const
    {
        return impl_->parent_context_;
    }

    void call_context::set_next( call_context *parent )
    {
        impl_->parent_context_ = parent;
    }

    void call_context::set_done_handler(google::protobuf::Closure *closure)
    {
        impl_->closure_ = closure;
    }

    const lowlevel_unit *call_context::get_lowlevel_message( ) const
    {
        return impl_->llu_;
    }

    lowlevel_unit *call_context::get_lowlevel_message( )
    {
        return impl_->llu_;
    }

    const vtrc_rpc_lowlevel::options *call_context::get_call_options( ) const
    {
        return impl_->opts_;
    }

    void call_context::set_call_options(const vtrc_rpc_lowlevel::options &opts)
    {
        impl_->opts_ = &opts;
    }

    call_context::~call_context( )
    {
        delete impl_;
    }

}}

