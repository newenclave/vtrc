#include <map>
#include <string>

#include "lukki-db-application.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include "vtrc-common/vtrc-rpc-channel.h"

#include "vtrc-server/vtrc-channels.h"

#include "protocol/lukkidb.pb.h"

#include "google/protobuf/descriptor.h"

#include "vtrc-bind.h"
#include "vtrc-ref.h"

#include "boost/asio.hpp"

namespace lukki_db {

    using namespace vtrc;

    namespace {

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

            static void yks_closure( bool success, const std::string &mess,
                              ::google::protobuf::RpcController* controller,
                              vtrc::shared_ptr<common::closure_holder> holder)
            {
                if( !success ) {
                    controller->SetFailed( mess );
                }
            }

            void set(::google::protobuf::RpcController* controller,
                         const ::vtrc_example::value_set_req* request,
                         ::vtrc_example::empty* response,
                         ::google::protobuf::Closure* done)
            {
                vtrc::shared_ptr<common::closure_holder> holder(
                            common::closure_holder::create(done) );
                app_.set( request->name( ), request->value( ),
                          vtrc::bind( &this_type::yks_closure,
                                      _1, _2, controller, holder) );
            }

            void upd(::google::protobuf::RpcController* controller,
                            const ::vtrc_example::value_set_req* request,
                            ::vtrc_example::empty* response,
                            ::google::protobuf::Closure* done)
            {
                vtrc::shared_ptr<common::closure_holder> holder(
                            common::closure_holder::create(done) );
                app_.upd( request->name( ), request->value( ),
                          vtrc::bind( &this_type::yks_closure,
                                      _1, _2, controller, holder) );
            }

            void del(::google::protobuf::RpcController* controller,
                         const ::vtrc_example::name_req* request,
                         ::vtrc_example::empty* response,
                         ::google::protobuf::Closure* done)
            {
                vtrc::shared_ptr<common::closure_holder> holder(
                            common::closure_holder::create(done) );

                app_.del( request->name( ),
                          vtrc::bind( &this_type::yks_closure,
                                      _1, _2, controller, holder) );

            }

            template <typename ResType>
            static void mess_closure( bool success, const std::string &mess,
                     vtrc::shared_ptr<ResType> res, ResType* response,
                     ::google::protobuf::RpcController* controller,
                     vtrc::shared_ptr<common::closure_holder> holder )
            {
                if( !success ) {
                    controller->SetFailed( mess );
                } else {
                    response->Swap( res.get( ) );
                }
            }

            void get(::google::protobuf::RpcController* controller,
                         const ::vtrc_example::name_req* request,
                         ::vtrc_example::lukki_string_list* response,
                         ::google::protobuf::Closure* done)
            {
                vtrc::shared_ptr<common::closure_holder> holder(
                            common::closure_holder::create(done) );

                typedef vtrc_example::lukki_string_list res_type;

                vtrc::shared_ptr<res_type> res(new res_type);

                app_.get( request->name( ), res,
                           vtrc::bind( &this_type::mess_closure<res_type>,
                                  _1, _2,
                                  res, response, controller, holder) );
            }

            void stat(::google::protobuf::RpcController* controller,
                         const ::vtrc_example::empty* request,
                         ::vtrc_example::db_stat* response,
                         ::google::protobuf::Closure* done)
            {

                vtrc::shared_ptr<common::closure_holder> holder(
                            common::closure_holder::create(done) );

                typedef vtrc_example::db_stat res_type;

                vtrc::shared_ptr<res_type> res(new res_type);
                app_.stat( res, vtrc::bind( &this_type::mess_closure<res_type>,
                                         _1, _2,
                                         res, response, controller, holder) );
            }

            void exist(::google::protobuf::RpcController* controller,
                     const ::vtrc_example::name_req* request,
                     ::vtrc_example::exist_res* response,
                     ::google::protobuf::Closure* done )
            {
                vtrc::shared_ptr<common::closure_holder> holder(
                            common::closure_holder::create(done) );

                typedef vtrc_example::exist_res res_type;
                vtrc::shared_ptr<res_type> res (new res_type);
                app_.exist( request->name( ), res,
                            vtrc::bind( &this_type::mess_closure<res_type>,
                                        _1, _2,
                                        res, response, controller, holder ));

            }
            void subscribe(::google::protobuf::RpcController* controller,
                     const ::vtrc_example::empty* request,
                     ::vtrc_example::empty* response,
                     ::google::protobuf::Closure* done)
            {
                common::closure_holder holder(done);
                app_.subscribe_client( connection_ );
            }
        };
    }

    struct  application::impl {

        common::thread_pool             db_thread_;
        db_type                         db_;
        vtrc_example::db_stat           db_stat_;
        boost::asio::io_service::strand event_queue_;

        std::vector<server::listener_sptr>                listeners_;

        typedef vtrc::shared_ptr<common::rpc_channel> channel_sptr;
        typedef std::pair<
            common::connection_iface_wptr, channel_sptr
        > connection_channel_pair;

        typedef std::list<connection_channel_pair> subscriber_list_type;

        subscriber_list_type subscribers_;

        impl( boost::asio::io_service &rpc_service )
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
                  const operation_closure &closure)
        {
            if( db_.find( name ) != db_.end( ) ) {
                if( closure ) closure( false, "Record already exists" );
            } else {
                db_stat_.set_set_requests( db_stat_.set_requests( ) + 1 );
                db_[name] = value;
                db_stat_.set_total_records( db_.size( ) );
                send_event( name , true );
                if( closure ) closure( true, "Success" );
            }
        }

        void upd( const std::string &name,
                  const vtrc_example::lukki_string_list &value,
                  const operation_closure &closure)
        {
            db_stat_.set_upd_requests( db_stat_.upd_requests( ) + 1 );
            db_type::iterator f( db_.find( name ));

            if( f == db_.end( ) ) {
                if( closure ) closure( false, "Record was not found" );
            } else {
                f->second.CopyFrom( value );
                send_event( name , true );
                if( closure ) closure( true, "Success" );
            }
        }

        void get( const std::string &name,
                  vtrc::shared_ptr<vtrc_example::lukki_string_list> value,
                  const operation_closure &closure)
        {
            db_stat_.set_get_requests( db_stat_.get_requests( ) + 1 );
            db_type::const_iterator f( db_.find( name ));
            if( f == db_.end( ) ) {
                if( closure ) closure( false, "Record was not found" );
            } else {
                value->CopyFrom( f->second );
                if( closure ) closure( true, "Success" );
            }
        }

        void del( const std::string &name,
                  const operation_closure &closure)
        {
            db_stat_.set_del_requests( db_stat_.del_requests( ) + 1 );
            db_type::iterator f( db_.find( name ));
            if( f == db_.end( ) ) {
                if( closure ) closure( false, "Record was not found" );
            } else {
                db_.erase( f );
                send_event( name , false );
                if( closure ) closure( true, "Success" );
            }
        }

        void stat( vtrc::shared_ptr<vtrc_example::db_stat> stat,
                   const operation_closure &closure)
        {
            stat->CopyFrom( db_stat_ );
            if( closure ) closure( true, "Success" );
        }

        void exist( const std::string &name,
                    vtrc::shared_ptr<vtrc_example::exist_res> res,
                    const operation_closure &closure)
        {
            res->set_value( db_.find( name ) != db_.end( ) );
            if( closure ) closure( true, "Success" );
        }

        /// events

        void send_event( const std::string &record, bool changed )
        {
            event_queue_.post( vtrc::bind( &impl::send_event_impl, this,
                                           record, changed) );
        }

        void send_event( vtrc::shared_ptr<common::rpc_channel> channel,
                         const std::string &record, bool changed)
        {
            try {
                vtrc_example::lukki_events_Stub s(channel.get( ));
                vtrc_example::name_req req;
                vtrc_example::empty    res;
                req.set_name( record );
                if(changed) {
                    s.value_changed( NULL, &req, &res, NULL );
                } else {
                    s.value_removed( NULL, &req, &res, NULL );
                }
            } catch( ... ) {
                ;;;
            }
        }

        void send_event_impl( const std::string &record, bool changed )
        {
            subscriber_list_type::iterator b(subscribers_.begin( ));
            while( b != subscribers_.end( ) ) {
                common::connection_iface_sptr lck(b->first.lock( ));
                if( lck ) {
                    send_event( b->second, record, changed );
                    b++;
                } else {
                    b = subscribers_.erase( b );
                }
            }
        }

        void send_subscribed( vtrc::shared_ptr<common::rpc_channel> channel )
        {
            try {
                vtrc_example::lukki_events_Stub s(channel.get( ));
                vtrc_example::empty r;
                s.subscribed( NULL, &r, &r, NULL );
            } catch( ... ) {
                ;;;
            }
        }

        void subscribe_impl( common::connection_iface_wptr conn )
        {
            common::connection_iface_sptr lck(conn.lock( ));
            if( lck ) {
                vtrc::shared_ptr<common::rpc_channel> channel
                    (server::channels::unicast::create_event_channel
                            ( lck, true ));
                subscribers_.push_back( std::make_pair(conn, channel) );
                send_subscribed( channel );
            }
        }

        void subscribe_client( vtrc::common::connection_iface* conn )
        {
            event_queue_.post( vtrc::bind(&impl::subscribe_impl, this,
                               conn->weak_from_this( ) ));
        }

    };

    application::application( vtrc::common::pool_pair &pp )
        :vtrc::server::application(pp)
        ,impl_(new impl(pp.get_rpc_service( )))
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
                                        ( new lukki_db_impl( *this, conn ) );
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

    void application::set( const std::string &name,
              const vtrc_example::lukki_string_list &value,
              const operation_closure &closure)
    {
        impl_->db_thread_.get_io_service( ).post(
                    vtrc::bind( &impl::set, impl_,name, value, closure));
    }

    void application::upd( const std::string &name,
              const vtrc_example::lukki_string_list &value,
              const operation_closure &closure)
    {
        impl_->db_thread_.get_io_service( ).post(
                    vtrc::bind( &impl::upd, impl_,name, value, closure));
    }

    void application::get( const std::string &name,
                       vtrc::shared_ptr<vtrc_example::lukki_string_list> value,
                       const operation_closure &closure)
    {
        impl_->db_thread_.get_io_service( ).post(
                    vtrc::bind( &impl::get, impl_, name,
                                value, closure));
    }

    void application::del( const std::string &name,
                           const operation_closure &closure)
    {
        impl_->db_thread_.get_io_service( ).post(
                    vtrc::bind( &impl::del, impl_, name, closure));
    }

    void application::stat(vtrc::shared_ptr<vtrc_example::db_stat> stat,
                            const operation_closure &closure)
    {
        impl_->db_thread_.get_io_service( ).post(
               vtrc::bind( &impl::stat, impl_, stat, closure));
    }

    void application::exist( const std::string &name,
                vtrc::shared_ptr<vtrc_example::exist_res> res,
                const operation_closure &closure)
    {
        impl_->db_thread_.get_io_service( ).post(
               vtrc::bind( &impl::exist, impl_, name, res, closure));
    }

    void application::subscribe_client( vtrc::common::connection_iface* conn )
    {
        impl_->subscribe_client( conn );
    }

}

