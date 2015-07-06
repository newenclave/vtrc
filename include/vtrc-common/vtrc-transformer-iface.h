#ifndef VTRC_TRANSFORMER_IFACE_H
#define VTRC_TRANSFORMER_IFACE_H

#include <stdlib.h>
#include <string>

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
        }
        transformer_iface *create( unsigned id, const char *key, size_t length);

        void generate_key_infos( const std::string &key,
                                 std::string &s1, std::string &s2,
                                 std::string &result );

        void create_key( const std::string &key,
                         const std::string &s1, const std::string &s2,
                               std::string &result );

    }
}}

#endif // VTRC_TRANSFORMER_IFACE_H
