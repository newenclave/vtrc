#include "vtrc-call-context.h"

namespace vtrc { namespace common {

    struct call_context::impl {

    };

    call_context::call_context( )
        :impl_(new impl)
    {}

    call_context::call_context( const call_context &other )
        :impl_(new impl(*other.impl_))
    {}

    call_context &call_context::operator = ( const call_context &other )
    {
        *impl_ = *other.impl_;
        return *this;
    }

    call_context::~call_context( )
    {
        delete impl_;
    }

}}

