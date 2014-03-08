
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include "vtrc-common/vtrc-mutex.h"

#include "vtrc-protocol-layer-s.h"

#include "vtrc-monotonic-timer.h"
#include "vtrc-data-queue.h"
#include "vtrc-hasher-iface.h"
#include "vtrc-transformer-iface.h"

#include "vtrc-transport-iface.h"

#include "vtrc-common/vtrc-rpc-service-wrapper.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-rpc-controller.h"

#include "vtrc-application.h"

#include "protocol/vtrc-errors.pb.h"
#include "protocol/vtrc-auth.pb.h"
#include "protocol/vtrc-rpc-lowlevel.pb.h"

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

    }

    namespace data_queue = common::data_queue;

    struct protocol_layer::impl {

        typedef impl this_type;

        application             &app_;
        common::transport_iface *connection_;
        protocol_layer          *parent_;
        bool                     ready_;

        service_map              services_;
        shared_mutex             services_lock_;

        typedef boost::function<void (void)> stage_function_type;
        stage_function_type  stage_function_;

        impl( application &a, common::transport_iface *c )
            :app_(a)
            ,connection_(c)
            ,ready_(false)
        {
            stage_function_ =
                    boost::bind( &this_type::on_client_selection, this );
        }

        common::rpc_service_wrapper_sptr get_service( const std::string &name )
        {
            upgradable_lock l( services_lock_ );
            common::rpc_service_wrapper_sptr result;
            service_map::iterator f( services_.find( name ) );

            if( f != services_.end( ) ) {
                result = f->second;
            } else {
                result = app_.get_service_by_name( connection_, name );
                if( result ) {
                    upgrade_to_unique ul( l );
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
                        common::hasher::create_by_index( cs.hash( ) ));
            parent_->change_hash_maker(
                        common::hasher::create_by_index( cs.hash( ) ));

            vtrc_auth::transformer_setup ts;

            ts.set_ready( true );

            stage_function_ =
                    boost::bind( &this_type::on_rcp_call_ready, this );

            send_proto_message( ts );

        }

        void get_pop_message( gpb::Message &capsule )
        {
            std::string &mess(parent_->message_queue( ).front( ));
            bool check = check_message_hash(mess);
            if( !check ) {
                connection_->close( );
                return;
            }
            parse_message( mess, capsule );
            pop_message( );
        }

        void closure( common::rpc_controller_sptr controller,
                           boost::shared_ptr <
                                vtrc_rpc_lowlevel::lowlevel_unit
                           > llu)
        {
            ;;;
        }

        void make_call_impl( boost::shared_ptr <
                              vtrc_rpc_lowlevel::lowlevel_unit> llu )
        {

            protocol_layer::context_holder ch( parent_, llu.get( ) );

            common::rpc_service_wrapper_sptr
                    service(get_service(llu->call().service()));

            if( !service ) {
                throw vtrc::common::exception( vtrc_errors::ERR_BAD_FILE,
                                               "Service not found");
            }

            gpb::MethodDescriptor const *meth
                (service->get_method(llu->call( ).method( )));

            if( !meth ) {
                throw vtrc::common::exception( vtrc_errors::ERR_NO_FUNC );
            }

            boost::shared_ptr<gpb::Message> req
                (service->service( )->GetRequestPrototype( meth ).New( ));

            req->ParseFromString( llu->request( ) );

            boost::shared_ptr<gpb::Message> res
                (service->service( )->GetResponsePrototype( meth ).New( ));
            res->ParseFromString( llu->response( ) );

            common::rpc_controller_sptr controller
                                (boost::make_shared<common::rpc_controller>( ));

            boost::shared_ptr<gpb::Closure> clos
                    (gpb::NewPermanentCallback( this, &this_type::closure,
                                                controller, llu ));


            service->service( )
                    ->CallMethod( meth, controller.get( ),
                                  req.get( ), res.get( ), clos.get( ) );

            if( controller->Failed( ) ) {
                throw vtrc::common::exception( vtrc_errors::ERR_INTERNAL,
                                               controller->ErrorText( ));
            }

            llu->set_response( res->SerializeAsString( ) );
        }

        void make_call( boost::shared_ptr <
                            vtrc_rpc_lowlevel::lowlevel_unit> llu )
        {
            bool failed = false;
            unsigned errorcode = 0;
            try {
                make_call_impl( llu );
            } catch ( const vtrc::common::exception &ex ) {
                errorcode = ex.code( );
                llu->mutable_info( )
                        ->mutable_error( )
                        ->set_additional( ex.additional( ) );
                failed = true;
            } catch ( const std::exception &ex ) {
                errorcode = vtrc_errors::ERR_INTERNAL;
                llu->mutable_info( )
                        ->mutable_error( )
                        ->set_additional( ex.what( ) );
                failed = true;
            } catch ( ... ) {
                errorcode = vtrc_errors::ERR_UNKNOWN;
                llu->mutable_info( )
                        ->mutable_error( )
                        ->set_additional( "..." );
                failed = true;
            }

            llu->clear_request( );
            llu->clear_call( );
            if( failed ) {
                llu->clear_response( );
            }
            send_proto_message( *llu );
        }

        void on_rcp_call_ready( )
        {
            while( !parent_->message_queue( ).empty( ) ) {
                boost::shared_ptr <
                            vtrc_rpc_lowlevel::lowlevel_unit
                        > llu(new vtrc_rpc_lowlevel::lowlevel_unit);
                get_pop_message( *llu );
                switch (llu->info( ).message_type( )) {
                case vtrc_rpc_lowlevel::message_info::MESSAGE_CALL:
                    make_call( llu );
                    break;
                default:
                    break;
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

    protocol_layer::protocol_layer( application &a,
                                    common::transport_iface *connection )
        :common::protocol_layer(connection)
        ,impl_(new impl(a, connection))
    {
        impl_->parent_ = this;
    }

    protocol_layer::~protocol_layer( )
    {
        delete impl_;
    }

    void protocol_layer::init( )
    {
        impl_->init( );
    }

    void protocol_layer::on_data_ready( )
    {
        impl_->data_ready( );
    }

    bool protocol_layer::ready( ) const
    {
        return impl_->ready( );
    }

}}

