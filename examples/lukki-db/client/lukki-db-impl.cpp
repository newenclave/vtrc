#include "lukki-db-iface.h"
#include "protocol/lukkidb.pb.h"

#include "vtrc/client/client.h"
#include "vtrc/common/stub-wrapper.h"

namespace {

    namespace gpb = google::protobuf;
    typedef vtrc_example::lukki_db_Stub stub_type;
    using namespace vtrc;

    struct lukki_db_impl: public interfaces::lukki_db {

        typedef common::rpc_channel channel_type;
        mutable vtrc::common::stub_wrapper<stub_type>  wrap_stub_;

        vtrc::shared_ptr<channel_type> get_channel( client::vtrc_client &c )
        {
            return vtrc::shared_ptr<channel_type>(c.create_channel(  ));
        }

        lukki_db_impl( vtrc::shared_ptr<vtrc::client::vtrc_client> c )
            :wrap_stub_(get_channel( *c ))
        {
            wrap_stub_.call( &stub_type::init );
        }

        void set( const std::string &name,
                  const std::vector<std::string> &value ) const
        {
            vtrc_example::value_set_req req;
            req.set_name( name );

            typedef std::vector<std::string>::const_iterator citer;

            for(citer b(value.begin( )), e(value.end( )); b!=e; ++b ) {
                req.mutable_value( )->add_value( *b );
            }

            wrap_stub_.call_request( &stub_type::set, &req );
        }

        void upd( const std::string &name,
                  const std::vector<std::string> &value ) const
        {
            vtrc_example::value_set_req req;
            req.set_name( name );

            typedef std::vector<std::string>::const_iterator citer;

            for(citer b(value.begin( )), e(value.end( )); b!=e; ++b ) {
                req.mutable_value( )->add_value( *b );
            }

            wrap_stub_.call_request( &stub_type::upd, &req );
        }

        std::vector<std::string> get(const std::string &name) const
        {
            vtrc_example::name_req          req;
            vtrc_example::lukki_string_list res;
            req.set_name( name );

            wrap_stub_.call( &stub_type::get, &req, &res );

            return std::vector<std::string>( res.value( ).begin( ),
                                             res.value( ).end( ));
        }

        void del( const std::string &name ) const
        {
            vtrc_example::name_req      req;
            req.set_name( name );
            wrap_stub_.call_request( &stub_type::del, &req );
        }


        vtrc_example::db_stat stat( ) const
        {
            vtrc_example::db_stat       res;
            wrap_stub_.call_response( &stub_type::stat, &res );
            return res;
        }

        bool exist( const std::string &name ) const
        {
            vtrc_example::name_req      req;
            vtrc_example::exist_res     res;

            req.set_name( name );

            wrap_stub_.call( &stub_type::exist, &req, &res );

            return res.value( );
        }

        void subscribe( ) const
        {
            wrap_stub_.call( &stub_type::subscribe );
        }

    };

}

namespace interfaces {

    lukki_db *create_lukki_db(vtrc::shared_ptr<vtrc::client::vtrc_client> clnt)
    {
        return new lukki_db_impl( clnt );
    }
}

