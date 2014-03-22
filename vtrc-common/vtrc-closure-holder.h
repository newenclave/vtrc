#ifndef VTRC_CLOSURE_HOLDER_H
#define VTRC_CLOSURE_HOLDER_H

#include <google/protobuf/stubs/common.h>

namespace vtrc { namespace common {

    class closure_holder {
        google::protobuf::Closure *done_;

        closure_holder( const closure_holder &other );
        closure_holder& operator = ( const closure_holder &other );

    public:

        closure_holder( google::protobuf::Closure *done )
            :done_(done)
        {}

        ~closure_holder( )
        {
            if( done_ ) done_->Run( );
        }

    };
}}

#endif // VTRCCLOSUREHOLDER_H