#ifndef LUKKI_DB_IFACE_H
#define LUKKI_DB_IFACE_H

#include <string>
#include <vector>

#include "vtrc-memory.h"

namespace vtrc { namespace client {
    class vtrc_client;
}}

namespace interfaces {
    struct luki_db {
        virtual ~luki_db( );
        void set( const std::string &name, const std::string &value ) const = 0;

        void set( const std::string &name,
                  const std::vector<std::string> &value ) const = 0;

        void del( const std::string &name ) const = 0;
    };

    luki_db *create_lukki_db(vtrc::shared_ptr<vtrc::client::vtrc_client> clnt);
}

#endif // LUKKIDBIFACE_H
