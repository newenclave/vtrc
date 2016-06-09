#include <map>
#include <string>
#include <iostream>

#include "lukki-db-application.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-common/vtrc-stub-wrapper.h"

#include "vtrc-server/vtrc-channels.h"

#include "protocol/lukkidb.pb.h"

#include "google/protobuf/descriptor.h"

#include "vtrc-bind.h"
#include "vtrc-ref.h"

#include "vtrc-asio.h"

namespace lukki_db {

    using namespace vtrc;

    namespace {

        namespace gpb = google::protobuf;

        typedef std::map<std::string, vtrc_example::lukki_string_list> db_type;

        class lukki_db_impl: public vtrc_example::lukki_db {

            typedef lukki_db_impl this_type;

            application              &app_;
            common::connection_iface *connection_;

        public:

            lukki_db_impl( application &app, common::connection_iface *c)
                :app_(app)
                ,connection_(c)
            { }

        private:

            void init(::google::protobuf::RpcController* /*controller*/,
                         const ::vtrc_example::empty* /*request*/,
                         ::vtrc_example::empty* /*response*/,
                         ::google::protobuf::Closure* /*done*/)
            { }

            void set(::google::protobuf::RpcController* controller,
                         const ::vtrc_example::value_set_req* request,
                         ::vtrc_example::empty* /*response*/,
                         ::google::protobuf::Closure* /*done*/)
            {
                app_.set( request->name( ), request->value( ), controller );
            }

            void upd(::google::protobuf::RpcController* controller,
                            const ::vtrc_example::value_set_req* request,
                            ::vtrc_example::empty* /*response*/,
                            ::google::protobuf::Closure* /*done*/)
            {
                app_.upd( request->name( ), request->value( ), controller );
            }

            void del(::google::protobuf::RpcController* controller,
                         const ::vtrc_example::name_req* request,
                         ::vtrc_example::empty* /*response*/,
                         ::google::protobuf::Closure* /*done*/)
            {
                app_.del( request->name( ), controller );
            }

            void get(::google::protobuf::RpcController* controller,
                         const ::vtrc_example::name_req* request,
                         ::vtrc_example::lukki_string_list* response,
                         ::google::protobuf::Closure* /*done*/)
            {
                app_.get( request->name( ), response, controller );
            }

            void stat(::google::protobuf::RpcController* /*controller*/,
                         const ::vtrc_example::empty* /*request*/,
                         ::vtrc_example::db_stat* response,
                         ::google::protobuf::Closure* /*done*/)
            {

                app_.stat( response );
            }

            void exist(::google::protobuf::RpcController* controller,
                     const ::vtrc_example::name_req* request,
                     ::vtrc_example::exist_res* response,
                     ::google::protobuf::Closure* /*done*/)
            {
                app_.exist( request->name( ), response );

            }
            void subscribe(::google::protobuf::RpcController* /*controller*/,
                     const ::vtrc_example::empty* /*request*/,
                     ::vtrc_example::empty* /*response*/,
                     ::google::protobuf::Closure* /*done*/)
            {
                app_.subscribe_client( connection_ );
            }
        };
    }

    struct  application::impl {

        enum { RECORD_DELETED, RECORD_ADDED, RECORD_CHANGED };

        common::thread_pool             db_thread_;
        db_type                         db_;
        vtrc_example::db_stat           db_stat_;
        VTRC_ASIO::io_service::strand event_queue_;

        std::vector<server::listener_sptr>  listeners_;

        typedef vtrc::shared_ptr<common::rpc_channel> channel_sptr;
        typedef std::pair<
            common::connection_iface_wptr, channel_sptr
        > connection_channel_pair;

        typedef std::list<connection_channel_pair> subscriber_list_type;

        subscriber_list_type subscribers_;

        impl( VTRC_ASIO::io_service &rpc_service )
            :db_thread_(1)
            ,event_queue_(rpc_service)
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

        void set( const std::string &name,
                  const vtrc_example::lukki_string_list &value,
                  gpb::RpcController *controller )
        {
            if( db_.find( name ) != db_.end( ) ) {
                controller->SetFailed( "Record exists" );
            } else {
                db_stat_.set_set_requests( db_stat_.set_requests( ) + 1 );
                db_[name] = value;
                db_stat_.set_total_records( db_.size( ) );
                send_event( name , RECORD_ADDED );
            }
        }

        void upd( const std::string &name,
                  const vtrc_example::lukki_string_list &value,
                  gpb::RpcController *controller )
        {
            db_stat_.set_upd_requests( db_stat_.upd_requests( ) + 1 );
            db_type::iterator f( db_.find( name ));

            if( f == db_.end( ) ) {
                controller->SetFailed( "Record was not found" );
            } else {
                f->second.CopyFrom( value );
                send_event( name , RECORD_CHANGED );
            }
        }

        void get( const std::string &name,
                  vtrc_example::lukki_string_list *value,
                  gpb::RpcController *controller )
        {
            db_stat_.set_get_requests( db_stat_.get_requests( ) + 1 );
            db_type::const_iterator f( db_.find( name ));
            if( f == db_.end( ) ) {
                controller->SetFailed( "Record was not found" );
            } else {
                value->CopyFrom( f->second );
            }
        }

        void del( const std::string &name,
                  gpb::RpcController *controller )
        {
            db_stat_.set_del_requests( db_stat_.del_requests( ) + 1 );
            db_type::iterator f( db_.find( name ));
            if( f == db_.end( ) ) {
                controller->SetFailed( "Record was not found" );
            } else {
                db_.erase( f );
                send_event( name , RECORD_DELETED );
            }
        }

        void stat( vtrc_example::db_stat *stat )
        {
            stat->CopyFrom( db_stat_ );
        }

        void exist( const std::string &name, vtrc_example::exist_res *res )
        {
            res->set_value( db_.find( name ) != db_.end( ) );
        }

        /// events
        void send_event( const std::string &record, unsigned changed )
        {
            event_queue_.post( vtrc::bind( &impl::send_event_impl, this,
                                           record, changed ) );
        }

        typedef vtrc_example::lukki_events_Stub       event_stub_type;
        typedef common::stub_wrapper<event_stub_type> stub_wrapper_type;


        void send_event( vtrc::shared_ptr<common::rpc_channel> channel,
                         const std::string &record, unsigned changed )
        {
            try {

                stub_wrapper_type s( channel );
                vtrc_example::name_req req;

                req.set_name( record );
                switch (changed) {
                case RECORD_DELETED:
                    s.call_request( &event_stub_type::value_removed, &req );
                    break;
                case RECORD_CHANGED:
                    s.call_request( &event_stub_type::value_changed, &req );
                    break;
                case RECORD_ADDED:
                    s.call_request( &event_stub_type::new_value, &req );
                    break;
                default:
                    break;
                }
            } catch( ... ) {
                ;;;
            }
        }

        void send_event_impl( const std::string &record, unsigned changed )
        {
            subscriber_list_type::iterator b(subscribers_.begin( ));
            while( b != subscribers_.end( ) ) {
                common::connection_iface_sptr lck(b->first.lock( ));
                if( lck ) {
                    send_event( b->second, record, changed );
                    ++b;
                } else {
                    b = subscribers_.erase( b );
                }
            }
        }

        void send_subscribed( common::rpc_channel &channel )
        {
            try {
                stub_wrapper_type stub(&channel);
                stub.call( &event_stub_type::subscribed );
            } catch( ... ) {
                ;;;
            }
        }

        void subscribe_client( vtrc::common::connection_iface* conn )
        {
            vtrc::shared_ptr<common::rpc_channel> channel
                    (server::channels::unicast::create_event_channel
                     ( conn->shared_from_this( ), true ));

            subscribers_.push_back( std::make_pair( conn->weak_from_this( ),
                                                    channel) );
            send_subscribed( *channel );
        }

        void execute( vtrc::function<void(void)> func )
        {
            func( );
        }

    };

    application::application( vtrc::common::pool_pair &pp )
        :vtrc::server::application(pp)
        ,impl_(new impl(pp.get_rpc_service( )))
    { }

    application::~application( )
    {
        delete impl_;
    }

    class svc_wrapper: public common::rpc_service_wrapper {
        boost::asio::io_service::strand &dispatch_;
        common::connection_iface_wptr cl_;
    public:
        svc_wrapper( boost::asio::io_service::strand &dispatch,
                     common::connection_iface_wptr cl,
                     common::rpc_service_wrapper::service_type *svc )
            :common::rpc_service_wrapper(svc)
            ,dispatch_(dispatch)
            ,cl_(cl)
        { }

        void call( common::connection_iface_wptr cl,
                   const method_type *method,
                   google::protobuf::RpcController* controller,
                   const google::protobuf::Message* request,
                   google::protobuf::Message* response,
                   google::protobuf::Closure* done )
        {
            common::connection_iface_sptr lock(cl.lock( ));
            if( lock ) {
                common::closure_holder _(done);
                call_default( method, controller, request, response, done );
            }
        }

        void call_method( const method_type *method,
                     google::protobuf::RpcController* controller,
                     const google::protobuf::Message* request,
                     google::protobuf::Message* response,
                     google::protobuf::Closure* done )
        {
            dispatch_.post( vtrc::bind( &svc_wrapper::call, this, cl_,
                                        method, controller, request, response,
                                        done ) );
        }
    };

    vtrc::shared_ptr<common::rpc_service_wrapper>
         application::get_service_by_name( vtrc::common::connection_iface* conn,
                                  const std::string &service_name )
    {
        if( service_name == lukki_db_impl::descriptor( )->full_name( ) ) {
            return vtrc::make_shared<svc_wrapper>
                    ( vtrc::ref( impl_->event_queue_ ),
                      conn->weak_from_this( ),
                      new lukki_db_impl( *this, conn ) );
        }
        return vtrc::shared_ptr<common::rpc_service_wrapper>( );
    }

    void application::execute( common::protocol_iface::call_type call )
    {
        call( );
    }

    void application::attach_start_listener( server::listener_sptr listen )
    {
        listen->on_new_connection_connect(
                    vtrc::bind( &impl::on_new_connection, impl_,
                                 listen, vtrc::placeholders::_1 ));

        listen->on_stop_connection_connect(
                    vtrc::bind( &impl::on_stop_connection, impl_,
                                 listen, vtrc::placeholders::_1 ));

        listen->start( );
        impl_->listeners_.push_back( listen );
    }

    /// as we have only one thread for operations with our DB,
    /// pass the calls to this thread
    /// so we don't need locks for DB access
    void application::set( const std::string &name,
                           const vtrc_example::lukki_string_list &value,
                           gpb::RpcController *controller  )
    {
        impl_->set( name, value, controller );
    }

    void application::upd( const std::string &name,
              const vtrc_example::lukki_string_list &value,
                           gpb::RpcController *controller )
    {
        impl_->upd( name, value, controller );
    }

    void application::get( const std::string &name,
                           vtrc_example::lukki_string_list *value,
                           gpb::RpcController *controller )
    {
        impl_->get( name, value, controller );
    }

    void application::del( const std::string &name,
                           gpb::RpcController *controller )
    {
        impl_->del( name, controller );
    }

    void application::stat( vtrc_example::db_stat *stat )
    {
        impl_->stat( stat );
    }

    void application::exist( const std::string &name,
                             vtrc_example::exist_res *res )
    {
        impl_->exist( name, res );
    }

    void application::subscribe_client( vtrc::common::connection_iface* conn )
    {
        impl_->subscribe_client( conn );
    }

}

