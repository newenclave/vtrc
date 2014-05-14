#ifndef LUKKI_DB_IFACE_H
#define LUKKI_DB_IFACE_H

#include <string>
#include <vector>

#include "vtrc-memory.h"

namespace vtrc { namespace client {
    class vtrc_client;
}}

namespace vtrc_example {
    class db_stat;
}

namespace interfaces {
    struct luki_db {

        virtual ~luki_db( ) { }

        virtual void set( const std::string &name,
                          const std::vector<std::string> &value ) const = 0;

        virtual void upd( const std::string &name,
                          const std::vector<std::string> &value ) const = 0;

        virtual std::vector<std::string> get(const std::string &name) const = 0;

        virtual void del( const std::string &name ) const = 0;

        virtual vtrc_example::db_stat stat( ) const = 0;
        virtual bool exist( const std::string &name ) const = 0;
        virtual void subscribe( ) const = 0;
    };

    luki_db *create_lukki_db(vtrc::shared_ptr<vtrc::client::vtrc_client> clnt);
}

#endif // LUKKIDBIFACE_H
