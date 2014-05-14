#include "lukki-db-iface.h"
#include "protocol/lukkidb.pb.h"

#include "vtrc-client-base/vtrc-client.h"

namespace {

    namespace gpb = google::protobuf;
    typedef vtrc_example::lukki_db_Stub stub_type;

    struct lukki_db_impl: public interfaces::luki_db {

        vtrc::shared_ptr<google::protobuf::RpcChannel> channel_;
        mutable stub_type                              stub_;

        lukki_db_impl( vtrc::shared_ptr<vtrc::client::vtrc_client> c )
            :channel_(c->create_channel( ))
            ,stub_(channel_.get( ))
        { }

        void set( const std::string &name, const std::string &value ) const
        {
            vtrc_example::value_set_req req;
            vtrc_example::empty         res;
            req.set_name( name );
            req.mutable_value( )->add_value( value );
            stub_.set( NULL, &req, &res, NULL );
        }

        void set( const std::string &name,
                  const std::vector<std::string> &value ) const
        {
            vtrc_example::value_set_req req;
            vtrc_example::empty         res;
            req.set_name( name );

            typedef std::vector<std::string>::const_iterator citer;

            for(citer b(value.begin( )), e(value.end( )); b!=e; ++b ) {
                req.mutable_value( )->add_value( *b );
            }

            stub_.set( NULL, &req, &res, NULL );
        }

        void del( const std::string &name ) const
        {
            vtrc_example::name_req      req;
            vtrc_example::empty         res;
            req.set_name( name );
            stub_.del( NULL, &req, &res, NULL );
        }

    };

}

namespace interfaces {

    luki_db *create_lukki_db(vtrc::shared_ptr<vtrc::client::vtrc_client> clnt)
    {
        return new lukki_db_impl( clnt );
    }
}

