#include <algorithm>
#include <iostream>
#include <stdlib.h>

#include "vtrc-call-context.h"
#include "vtrc-connection-iface.h"
#include "vtrc-protocol-layer.h"

#include "protocol/vtrc-rpc-lowlevel.pb.h"

namespace vtrc { namespace common {

    typedef rpc::lowlevel_unit lowlevel_unit;

    struct call_context::impl {
        lowlevel_unit              *llu_;
        call_context               *parent_context_;
        const rpc::options         *opts_;
        bool                        impersonated_;
        google::protobuf::Closure  *done_;
        size_t                      depth_;
        impl(lowlevel_unit *llu)
            :llu_(llu)
            ,parent_context_(NULL)
            ,opts_(NULL)
            ,impersonated_(false)
            ,done_(NULL)
            ,depth_(1)
        { }

        ~impl( )
        { }
    };

    call_context::call_context( lowlevel_unit *lowlevel )
        :impl_(new impl(lowlevel))
    { }

    call_context::~call_context( )
    {
        delete impl_;
    }

    const call_context *call_context::get( )
    {
        return protocol_layer::get_call_context( );
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
        impl_->depth_          = parent ? parent->depth( ) + 1 : 1;
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

    const rpc::options *call_context::get_call_options( ) const
    {
        return impl_->opts_;
    }

    void call_context::set_call_options(const rpc::options *opts)
    {
        impl_->opts_ = opts;
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

    const size_t call_context::depth( ) const
    {
        return impl_->depth_;
    }

    const std::string &call_context::channel_data( ) const
    {
        return impl_->llu_->channel_data( );
    }

}}

