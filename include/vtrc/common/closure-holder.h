#ifndef VTRC_CLOSURE_HOLDER_H
#define VTRC_CLOSURE_HOLDER_H

#include "google/protobuf/stubs/common.h"
#include "vtrc-memory.h"

namespace vtrc { namespace common {

    class closure_holder {

        google::protobuf::Closure *done_;

        closure_holder( const closure_holder &other );
        closure_holder& operator = ( const closure_holder &other );

    public:

        static
        vtrc::shared_ptr<closure_holder> create(google::protobuf::Closure *done)
        {
            return vtrc::make_shared<closure_holder>( done );
        }

        closure_holder( google::protobuf::Closure *done )
            :done_(done)
        { }

        google::protobuf::Closure *release( )
        {
            google::protobuf::Closure *old = done_;
            done_ =  VTRC_NULL;
            return old;
        }

        ~closure_holder( )
        { try {
            if( done_ ) done_->Run( );
        } catch( ... ) {
            ;;;
        } }

    };
}}

#endif // VTRCCLOSUREHOLDER_H
