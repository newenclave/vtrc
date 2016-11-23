#include "vtrc-asio.h"

#include "vtrc-application.h"
#include "vtrc-common/vtrc-enviroment.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-connection-list.h"

#include "vtrc-common/vtrc-protocol-accessor-iface.h"

#include "vtrc-rpc-lowlevel.pb.h"

#include "vtrc-connection-impl.h"

#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-transport-dgram-impl.h"

#include "vtrc-common/vtrc-delayed-call.h"

#include "vtrc-chrono.h"

namespace vtrc { namespace server { namespace listeners {

    namespace {

        static const vtrc::uint64_t timer_interval = 60000000; /// microseconds
        static const vtrc::uint64_t client_timeout = 60000000; /// microseconds

        struct acceptor_base;
        typedef vtrc::common::delayed_call delayed_call;

        vtrc::uint64_t ticks_now( )
        {
            namespace chrono = vtrc::chrono;
            using chrono::duration_cast;
            using microsec = chrono::microseconds;

            chrono::system_clock::time_point n
                                   = chrono::high_resolution_clock::now( );
            return duration_cast<microsec>(n.time_since_epoch( )).count( );
        }

        struct client_info: public common::connection_iface {

            typedef acceptor_base parent_type;

            acceptor_base            *parent_;
            basio::ip::udp::endpoint  endpoint_;
            delayed_call              keeper_;
            vtrc::uint64_t            last_;

            typedef vtrc::shared_ptr<client_info> shared_type;
            client_info( acceptor_base *parent );
            void keeper_handler( const bsys::error_code &err );

            void start_keeper( )
            {
                namespace ph = vtrc::placeholders;
                keeper_.call_from_now(
                    vtrc::bind( &client_info::keeper_handler, this, ph::_1),
                    delayed_call::microseconds(timer_interval) ); /// settings?
            }

            std::string name( ) const       { }
            const std::string &id( )  const { }
            void init( )                    { }
            void close( )                   { }

            common::native_handle_type native_handle( ) const;

            void write( const char *data, size_t length ) { }
            void write( const char *data, size_t length,
                        const common::system_closure_type &success,
                        bool success_on_send ) { }

            common::protocol_iface &get_protocol( ) { }
            const common::protocol_iface &get_protocol( ) const { }
        };

        typedef std::map< basio::ip::udp::endpoint,
                          client_info::shared_type
                        > client_set;

        struct acceptor_base: public common::dgram_transport_impl {

            typedef basio::ip::udp::endpoint endpoint;

            client_set clients_;

            acceptor_base( basio::io_service &ios, size_t len )
                :common::dgram_transport_impl(ios, len)
            { }

            size_t size( ) const
            {
                return clients_.size( );
            }

            void drop_client( const endpoint &ep )
            {
                clients_.erase( ep );
            }

            client_info::shared_type get_client( const endpoint &ep )
            {
                client_set::iterator f = clients_.find( ep );
                if( f != clients_.end( ) ) {
                    return f->second;
                }
                return client_info::shared_type( );
            }

//            virtual void on_write( const bsys::error_code &, std::size_t ) { }
//            virtual void start( ) { }
//            virtual void on_read( const bsys::error_code &,
//                                  const basio::ip::udp::endpoint &,
//                                  std::uint8_t *, std::size_t ) { }

        };

        struct acceptor_master;

        struct acceptor_slave: public acceptor_base {

            acceptor_master *master_;
            endpoint ep_;

            typedef vtrc::shared_ptr<acceptor_slave> shared_type;

            acceptor_slave( acceptor_master *master );

            void start( )
            {
                ep_.address( ).is_v4( ) ? open_v4( ) : open_v6( );
                get_socket( ).bind( ep_ );
                ep_ = get_socket( ).local_endpoint( );
            }

            static shared_type create( acceptor_master *master );
        };

        typedef std::list<acceptor_slave::shared_type> slave_list;
        typedef std::map<size_t, slave_list> slaves_map;

        struct acceptor_master: public acceptor_base {

            endpoint ep_;

            slaves_map slaves_;

            acceptor_master(basio::io_service &ios, size_t len)
                :acceptor_base(ios, len)
            { }

            void add_slave(  )
            {
                slaves_[0].push_front(acceptor_slave::create( this ));
            }

            void start( )
            {
                ep_.address( ).is_v4( ) ? open_v4( ) : open_v6( );
            }

            void dec_slave( acceptor_slave *slave )
            {
                typedef slave_list::iterator itr;
                typedef acceptor_slave::shared_type slave_sptr;

                size_t old_size = slave->size( ) + 1;
                slave_list &lst(slaves_[old_size]);

                for( itr b(lst.begin( )), e(lst.end()); b != e; ++b ) {
                    if( b->get( ) == slave ) {
                        slave_sptr slv = *b;
                        lst.erase( b );
                        slaves_[old_size - 1].push_front( slv );
                        break;
                    }
                }

                if( lst.empty( ) ) {
                    slaves_.erase( old_size );
                }
            }

            void inc_slave( acceptor_slave *slave )
            {
                typedef slave_list::iterator itr;
                typedef acceptor_slave::shared_type slave_sptr;

                size_t old_size = slave->size( ) - 1;
                slave_list &lst(slaves_[old_size]);

                for( itr b(lst.begin( )), e(lst.end()); b != e; ++b ) {
                    if( b->get( ) == slave ) {
                        slave_sptr slv = *b;
                        lst.erase( b );
                        slaves_[old_size + 1].push_front( slv );
                        break;
                    }
                }

                if( lst.empty( ) ) {
                    slaves_.erase( old_size );
                }
            }

            endpoint &get_endpoint( )
            {
                return ep_;
            }
        };

        ///////////////////////////////// IMPL //////////////////////////////

        ///////// CLIENTS
        client_info::client_info( acceptor_base *parent )
            :parent_(parent)
            ,keeper_(parent_->get_io_service( ))
            ,last_(ticks_now( ))
        {
            start_keeper( );
        }

        common::native_handle_type client_info::native_handle( ) const
        {
            common::native_handle_type nh;
#ifdef _WIN32
            nh.value.win_handle =
                reinterpret_cast<native_handle_type::handle>
                    ((SOCKET)parent_->get_socket( ).native_handle( ));
#else
            nh.value.unix_fd = parent_->get_socket( ).native_handle( );
#endif
            return nh;
        }

        void client_info::keeper_handler( const bsys::error_code &err )
        {
            if( !err ) {
                vtrc::uint64_t now = ticks_now( );
                if( client_timeout > (now - last_) ) {
                    parent_->dispatch(
                        vtrc::bind( &parent_type::drop_client, parent_,
                                     endpoint_ ) );
                } else {
                    start_keeper( );
                }
            }
        }

        ///////// SLAVE
        acceptor_slave::shared_type acceptor_slave::create( acceptor_master *m )
        {
            acceptor_slave::shared_type r
                    = vtrc::make_shared<acceptor_slave>( m );
            return r;
        }

        acceptor_slave::acceptor_slave( acceptor_master *m )
            :acceptor_base(m->get_io_service( ), m->get_data( ).size( ))
            ,master_(m)
            ,ep_(master_->get_endpoint( ).address( ), 0)
        { }

        /////////////////////////////////////////////////////////////////////

        class udp_listener: public server::listener {

        };
    }

    namespace udp {

        listener_sptr create( application &app,
                              const std::string &address,
                              unsigned short service )
        {
            return listener_sptr( );
        }

        listener_sptr create( application &app,
                              const rpc::session_options &opts,
                              const std::string &address,
                              unsigned short service )
        {
            return listener_sptr( );
        }

    }


}}}
