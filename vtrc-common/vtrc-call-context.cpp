#include <stdlib.h>
#include "vtrc-call-context.h"
#include "vtrc-connection-iface.h"
#include "vtrc-protocol-layer.h"


namespace vtrc { namespace common {

    typedef vtrc_rpc::lowlevel_unit lowlevel_unit;

    struct call_context::impl {
        lowlevel_unit              *llu_;
        call_context               *parent_context_;
        const vtrc_rpc::options    *opts_;
        bool                        impersonated_;
        google::protobuf::Closure  *done_;
        impl(lowlevel_unit *llu)
            :llu_(llu)
            ,opts_(NULL)
            ,impersonated_(false)
            ,done_(NULL)
        { }
    };

    call_context::call_context( lowlevel_unit *lowlevel )
        :impl_(new impl(lowlevel))
    { }

    const call_context *call_context::get( connection_iface *iface )
    {
        return iface->get_call_context( );
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

    const lowlevel_unit *call_context::get_lowlevel_message( ) const
    {
        return impl_->llu_;
    }

    lowlevel_unit *call_context::get_lowlevel_message( )
    {
        return impl_->llu_;
    }

    void call_context::set_impersonated( bool value )
    {
        impl_->impersonated_ = value;
    }

    bool call_context::get_impersonated( ) const
    {
        return impl_->impersonated_;
    }

    const vtrc_rpc::options *call_context::get_call_options( ) const
    {
        return impl_->opts_;
    }

    void call_context::set_call_options(const vtrc_rpc::options &opts)
    {
        impl_->opts_ = &opts;
    }

    void call_context::set_done_closure( google::protobuf::Closure *done )
    {
        impl_->done_ = done;
    }

    google::protobuf::Closure *call_context::get_done_closure( )
    {
        return impl_->done_;
    }

    const google::protobuf::Closure *call_context::get_done_closure( ) const
    {
        return impl_->done_;
    }

    call_context::~call_context( )
    {
        delete impl_;
    }

}}

