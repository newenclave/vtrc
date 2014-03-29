
#include <boost/asio.hpp>

#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-bind.h"
#include "vtrc-function.h"

#include "vtrc-protocol-layer-s.h"

#include "vtrc-monotonic-timer.h"
#include "vtrc-data-queue.h"
#include "vtrc-hash-iface.h"
#include "vtrc-transformer-iface.h"

#include "vtrc-transport-iface.h"

#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-rpc-controller.h"
#include "vtrc-common/vtrc-call-context.h"

#include "vtrc-application.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-auth.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

#include "vtrc-chrono.h"
#include "vtrc-atomic.h"

namespace vtrc { namespace server {

    namespace gpb = google::protobuf;

    namespace {
        enum init_stage_enum {
             stage_begin             = 1
            ,stage_client_select     = 2
            ,stage_client_ready      = 3
        };

        typedef std::map <
            std::string,
            common::rpc_service_wrapper_sptr
        > service_map;

        typedef vtrc_rpc_lowlevel::lowlevel_unit     lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;
    }

    namespace data_queue = common::data_queue;

    struct atomic_counter_keeper {
        vtrc::atomic<unsigned> &value_;
        atomic_counter_keeper( vtrc::atomic<unsigned> &value )
            :value_(value)
        {
            ++value_;
        }

        ~atomic_counter_keeper( )
        {
            --value_;
        }
    };

    struct protocol_layer_s::impl {

        typedef impl             this_type;
        typedef protocol_layer_s parent_type;

        application             &app_;
        common::transport_iface *connection_;
        protocol_layer_s        *parent_;
        bool                     ready_;

        service_map              services_;
        shared_mutex             services_lock_;

        typedef vtrc::function<void (void)> stage_function_type;
        stage_function_type  stage_function_;

        vtrc::atomic<unsigned>              current_calls_;
        const unsigned                      maximum_calls_;

        impl( application &a, common::transport_iface *c,
              unsigned maximum_calls)
            :app_(a)
            ,connection_(c)
            ,ready_(false)
            ,current_calls_(0)
            ,maximum_calls_(maximum_calls)
        {
            stage_function_ =
                    vtrc::bind( &this_type::on_client_selection, this );
        }

        common::rpc_service_wrapper_sptr get_service( const std::string &name )
        {
            upgradable_lock lk( services_lock_ );
            common::rpc_service_wrapper_sptr result;
            service_map::iterator f( services_.find( name ) );

            if( f != services_.end( ) ) {
                result = f->second;
            } else {
                result = app_.get_service_by_name( connection_, name );
                if( result ) {
                    upgrade_to_unique ulk( lk );
                    services_.insert( std::make_pair( name, result ) );
                }
            }

            return result;
        }

        void pop_message( )
        {
            parent_->pop_message( );
        }

        void on_timer( boost::system::error_code const &error )
        {

        }

        bool check_message_hash( const std::string &mess )
        {
            return parent_->check_message( mess );
        }

        void write( const char *data, size_t length )
        {
            connection_->write( data, length );
        }

        void write( const char *data, size_t length ) const
        {
            connection_->write( data, length );
        }

        void send_proto_message( const gpb::Message &mess ) const
        {
            std::string s(mess.SerializeAsString( ));
            write( s.c_str( ), s.size( ) );
        }

        void on_client_selection( )
        {
            std::string &mess(parent_->message_queue( ).front( ));
            bool check = check_message_hash(mess);
            if( !check ) {
                connection_->close( );
                return;
            }
            vtrc_auth::client_selection cs;
            parse_message( mess, cs );

            pop_message( );

            parent_->change_hash_checker(
                        common::hash::create_by_index( cs.hash( ) ));
            parent_->change_hash_maker(
                        common::hash::create_by_index( cs.hash( ) ));

            vtrc_auth::transformer_setup ts;

            ts.set_ready( true );

            stage_function_ =
                    vtrc::bind( &this_type::on_rcp_call_ready, this );

            send_proto_message( ts );

        }

        bool get_pop_message( gpb::Message &capsule )
        {
            std::string &mess(parent_->message_queue( ).front( ));
            bool check = check_message_hash(mess);
            if( !check ) {
                connection_->close( );
                return false;
            }
            parse_message( mess, capsule );
            pop_message( );
            return true;
        }

        void send_busy( lowlevel_unit_type &llu )
        {
            if( llu.opt( ).wait( ) ) {
                llu.clear_call( );
                llu.clear_request( );
                llu.clear_response( );
                llu.mutable_error( )->set_code( vtrc_errors::ERR_BUSY );
                parent_->call_rpc_method( llu );
            }
            //llu->mutable_error( )->set_code(  );
        }


        void push_call( lowlevel_unit_sptr llu,
                        common::connection_iface_sptr /*conn*/ )
        {
            parent_->make_call( llu );
            --current_calls_;
        }

        void process_call( lowlevel_unit_sptr &llu )
        {
            if( ++current_calls_ <= maximum_calls_ ) {
                app_.get_rpc_service( ).post(
                        vtrc::bind( &this_type::push_call, this,
                                    llu, connection_->shared_from_this( )));
            } else {
                --current_calls_;
                send_busy( *llu );
            }
        }

        void push_event_answer( lowlevel_unit_sptr llu,
                                common::connection_iface_sptr /*conn*/ )
        {
            parent_->push_rpc_message( llu->id( ), llu );
        }

        void process_event_cb( lowlevel_unit_sptr &llu )
        {
            parent_->push_rpc_message( llu->id( ), llu );
//            app_.get_io_service( ).post(
//                        vtrc::bind( &this_type::push_event_answer, this,
//                                llu, connection_->shared_from_this( )));
        }

        void on_rcp_call_ready_( )
        {
            app_.get_io_service( ).post(
                        vtrc::bind(&this_type::on_rcp_call_ready, this));
        }

        void on_rcp_call_ready( )
        {
            typedef vtrc_rpc_lowlevel::message_info message_info;
            //std::cout << "call from client\n";
            while( !parent_->message_queue( ).empty( ) ) {

                lowlevel_unit_sptr llu(vtrc::make_shared<lowlevel_unit_type>());

                if(!get_pop_message( *llu )) return;

                if( llu->has_info( ) ) {
                    switch (llu->info( ).message_type( )) {
                    case message_info::MESSAGE_CALL:
                        process_call( llu );
                        break;
                    case message_info::MESSAGE_EVENT:
                    case message_info::MESSAGE_CALLBACK:
                    case message_info::MESSAGE_INSERTION_CALL:
                        process_event_cb( llu );
                        break;
                    default:
                        break;
                    }
                }
            }

            //connection_->close( );
        }

        void parse_message( const std::string &block, gpb::Message &mess )
        {
            parent_->parse_message( block, mess );
        }

        std::string first_message( )
        {
            vtrc_auth::init_protocol hello_mess;
            hello_mess.set_hello_message( "Tervetuloa!" );

            hello_mess.add_hash_supported( vtrc_auth::HASH_NONE     );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_16   );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_32   );
            hello_mess.add_hash_supported( vtrc_auth::HASH_CRC_64   );
            hello_mess.add_hash_supported( vtrc_auth::HASH_SHA2_256 );

            hello_mess.add_transform_supported( vtrc_auth::TRANSFORM_NONE );
            hello_mess.add_transform_supported( vtrc_auth::TRANSFORM_ERSEEFOR );
            return hello_mess.SerializeAsString( );
        }

        void init( )
        {
            static const std::string data(first_message( ));
            write(data.c_str( ), data.size( ));
        }

        void data_ready( )
        {
            stage_function_( );
        }

        bool ready( ) const
        {
            return true;
        }

    };

    protocol_layer_s::protocol_layer_s( application &a,
                                        common::transport_iface *connection,
                                        unsigned maximym_calls)
        :common::protocol_layer(connection, false)
        ,impl_(new impl(a, connection, maximym_calls))
    {
        impl_->parent_ = this;
    }

    protocol_layer_s::~protocol_layer_s( )
    {
        delete impl_;
    }

    void protocol_layer_s::init( )
    {
        impl_->init( );
    }

    void protocol_layer_s::on_data_ready( )
    {
        impl_->data_ready( );
    }

    bool protocol_layer_s::ready( ) const
    {
        return impl_->ready( );
    }

    common::rpc_service_wrapper_sptr protocol_layer_s::get_service_by_name(
                                                        const std::string &name)
    {
        return impl_->get_service(name);
    }

}}

