#ifndef VTRC_TRANSFORMER_IFACE_H
#define VTRC_TRANSFORMER_IFACE_H

#include <stdlib.h>
#include "vtrc-memory.h"

namespace vtrc { namespace common {

    struct transformer_iface {
        virtual ~transformer_iface( ) { }
        virtual void transform( char *data, size_t length ) = 0;
    };

    typedef vtrc::shared_ptr<transformer_iface> transformer_iface_sptr;

    namespace transformers {
        namespace none {
            transformer_iface *create( );
        }

        namespace erseefor {
            transformer_iface *create( const char *key, size_t length);
            void create_session_key( const std::string &key,
                                     std::string &result,
                                     std::string &s1,
                                     std::string &s2 );
        }
        transformer_iface *create( unsigned id, const char *key, size_t length);
    }
}}

#endif // VTRC_TRANSFORMER_IFACE_H
