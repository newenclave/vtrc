#include <map>
#include <string>

#include "lukki-db-application.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-connection-iface.h"

#include "protocol/lukkidb.pb.h"

#include "google/protobuf/descriptor.h"

#include "vtrc-bind.h"
#include "vtrc-ref.h"

namespace lukki_db {

    using namespace vtrc;

    namespace {

        typedef std::map<std::string, vtrc_example::lukki_string_list> db_type;

        class lukki_db_impl: public vtrc_example::lukki_db {
            application &app_;
        public:
            lukki_db_impl( application &app )
                :app_(app)
            { }
        };
    }

    struct  application::impl {

        common::pool_pair   &pool_;

        common::thread_pool db_thread_;
        db_type             db_;

        std::vector<server::listener_sptr> listeners_;

        impl( common::pool_pair &p )
            :pool_(p)
            ,db_thread_(1)
        { }

        void on_new_connection( server::listener_sptr l,
                                const common::connection_iface *c )
        {
            std::cout << "New connection: "
                      << "\n\tep:     " << l->name( )
                      << "\n\tclient: " << c->name( )
                      << "\n"
                        ;
        }

        void on_stop_connection( server::listener_sptr l,
                                 const common::connection_iface *c )
        {
            std::cout << "Close connection: "
                      << c->name( ) << "\n";
        }

    };

    application::application( vtrc::common::pool_pair &pp )
        :impl_(new impl(pp))
    {

    }

    application::~application( )
    {
        delete impl_;
    }

    vtrc::shared_ptr<common::rpc_service_wrapper>
         application::get_service_by_name( vtrc::common::connection_iface* conn,
                                  const std::string &service_name )
    {
        if( service_name == lukki_db_impl::descriptor( )->full_name( ) ) {
            return vtrc::make_shared<common::rpc_service_wrapper>
                                                ( new lukki_db_impl( *this ) );
        }
        return vtrc::shared_ptr<common::rpc_service_wrapper>( );
    }

    void application::attach_start_listener( server::listener_sptr listen )
    {
        listen->get_on_new_connection( ).connect(
                    vtrc::bind( &impl::on_new_connection, impl_,
                                listen, _1 ));

        listen->get_on_stop_connection( ).connect(
                    vtrc::bind( &impl::on_stop_connection, impl_,
                                listen, _1 ));
        listen->start( );
        impl_->listeners_.push_back( listen );
    }

}

