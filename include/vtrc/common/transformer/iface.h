#ifndef VTRC_TRANSFORMER_IFACE_H
#define VTRC_TRANSFORMER_IFACE_H

#include <stdlib.h>
#include <string>

#include "vtrc-memory.h"

namespace vtrc { namespace common {


    namespace transformer {

        struct iface {

            typedef vtrc::shared_ptr<iface> sptr;
            typedef vtrc::unique_ptr<iface> uptr;

            virtual ~iface( ) { }

            /// [in, out]
            virtual void transform( std::string &in_out_data ) = 0;

            //virtual void transform( char *data, size_t length ) = 0;
        };

        namespace none {
            iface *create( );
        }

        namespace erseefor {
            iface *create( const char *key, size_t length);
        }

        namespace chacha {
            iface *create( const char *key, size_t length);
        }

        iface *create( unsigned id, const char *key, size_t length);

        void generate_key_infos( const std::string &key,
                                 std::string &s1, std::string &s2,
                                 std::string &result );

        void create_key( const std::string &key,
                         const std::string &s1, const std::string &s2,
                               std::string &result );

    }
}}

#endif // VTRC_TRANSFORMER_IFACE_H
