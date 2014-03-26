
#include <boost/thread/tss.hpp>
#include "vtrc-thread.h"

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include "vtrc-atomic.h"
#include "vtrc-mutex.h"
#include "vtrc-mutex-typedefs.h"

#include "vtrc-protocol-layer.h"

#include "vtrc-data-queue.h"
#include "vtrc-hash-iface.h"
#include "vtrc-transformer-iface.h"

#include "vtrc-transport-iface.h"

#include "vtrc-condition-queues.h"
#include "vtrc-exception.h"
#include "vtrc-call-context.h"

#include "vtrc-rpc-controller.h"

#include "proto-helper/message-utilities.h"

#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "protocol/vtrc-errors.pb.h"

namespace vtrc { namespace common {

    namespace gpb = google::protobuf;

    namespace {

        typedef protocol_layer::message_queue_type message_queue_type;

        void raise_error( unsigned code )
        {
            throw vtrc::common::exception( code );
        }

        void raise_wait_error( wait_result_codes wr )
        {
            switch ( wr ) {
            case WAIT_RESULT_CANCELED:
                raise_error( vtrc_errors::ERR_CANCELED );
                break;
            case  WAIT_RESULT_TIMEOUT:
                raise_error( vtrc_errors::ERR_TIMEOUT );
                break;
            default:
                break;
            }
        }

        static const size_t maximum_message_length = 1024 * 1024;

        struct rpc_unit_index {
            gpb::uint64     id_;
            rpc_unit_index( gpb::uint64 id )
                :id_(id)
            { }
        };

        inline bool operator < ( const rpc_unit_index &lhs,
                                 const rpc_unit_index &rhs )
        {
            return lhs.id_ < rhs.id_;
        }

        typedef vtrc_rpc_lowlevel::lowlevel_unit ll_unit_type;
        typedef vtrc::shared_ptr<ll_unit_type>   ll_unit_sptr;

        typedef condition_queues<rpc_unit_index, ll_unit_sptr> rpc_queue_type;

        typedef boost::thread_specific_ptr<call_context> call_context_ptr;

        typedef std::map <
             const google::protobuf::MethodDescriptor *
            ,vtrc::shared_ptr<vtrc_rpc_lowlevel::options>
        > options_map_type;

    }

    struct protocol_layer::impl {

        typedef impl this_type;

        transport_iface             *connection_;
        protocol_layer              *parent_;
        hasher_iface_sptr            hash_maker_;
        hasher_iface_sptr            hash_checker_;
        transformer_iface_sptr       transformer_;
        transformer_iface_sptr       reverter_;

        data_queue::queue_base_sptr  queue_;

        rpc_queue_type               rpc_queue_;
        vtrc::atomic<uint64_t>       rpc_index_;

        call_context_ptr             context_;

        options_map_type             options_map_;
        mutable vtrc::shared_mutex   options_map_lock_;

        impl( transport_iface *c )
            :connection_(c)
            ,hash_maker_(common::hash::create_default( ))
            ,hash_checker_(common::hash::create_default( ))
//            ,transformer_(common::transformers::erseefor::create( "1234", 4 ))
//            ,reverter_(common::transformers::erseefor::create( "1234", 4 ))
            ,transformer_(common::transformers::none::create( ))
            ,reverter_(common::transformers::none::create( ))
            ,queue_(data_queue::varint::create_parser(maximum_message_length))
            ,rpc_index_(100)
        {}

        std::string prepare_data( const char *data, size_t length)
        {
            /* here is:
             *  message =
             *  <packed_size(data_length+hash_length)><hash(data)><data>
            */
            std::string result(queue_->pack_size(
                                length + hash_maker_->hash_size( )));

            result.append( hash_maker_->get_data_hash(data, length ));
            result.append( data, data + length );

            /*
             * message = transform( message )
            */

            lowlevel_unit_type llu;
            llu.ParseFromArray(data, length);

            transformer_->transform(
                            result.empty( ) ? NULL : &result[0],
                                            result.size( ) );
            return result;
        }

        void process_data( const char *data, size_t length )
        {
            if( length > 0 ) {
                std::string next_data(data, data + length);

                const size_t old_size = queue_->messages( ).size( );

                /*
                 * message = revert( message )
                */
                reverter_->transform( &next_data[0], next_data.size( ) );
                queue_->append( &next_data[0], next_data.size( ));

                queue_->process( );

                if( queue_->messages( ).size( ) > old_size ) {
                    parent_->on_data_ready( );
                }
            }

        }

        void drop_first( )
        {
            queue_->messages( ).pop_front( );
        }

        void send_data( const char *data, size_t length )
        {
            connection_->write( data, length );
        }

        void send_message( const gpb::Message &message )
        {
            std::string ser(message.SerializeAsString( ));
            send_data( ser.c_str( ), ser.size( ) );
        }

        uint64_t next_index( )
        {
            return ++rpc_index_;
        }

        // --------------- sett ----------------- //

        message_queue_type &message_queue( )
        {
            return queue_->messages( );
        }

        const message_queue_type &message_queue( ) const
        {
            return queue_->messages( );
        }

        void parse_message( const std::string &mess,
                            google::protobuf::Message &result )
        {
            const size_t hash_length = hash_checker_->hash_size( );
            const size_t diff_len    = mess.size( ) - hash_length;
            result.ParseFromArray( mess.c_str( ) + hash_length, diff_len );
        }

        bool check_message( const std::string &mess )
        {
            const size_t hash_length = hash_checker_->hash_size( );
            const size_t diff_len    = mess.size( ) - hash_length;

            bool result = false;

            if( mess.size( ) >= hash_length ) {
                result = hash_checker_->
                         check_data_hash( mess.c_str( ) + hash_length,
                                          diff_len,
                                          mess.c_str( ) );
            }
            return result;
        }

        call_context *release_call_context( )
        {
            call_context *old = context_.get( );
            context_.release( );
            return old;
        }

        call_context *reset_call_context( call_context *cc )
        {
            context_.reset( cc );
            return cc;
        }

        const call_context *get_call_context( ) const
        {
            return context_.get( );
        }

        call_context *get_call_context( )
        {
            return context_.get( );
        }

        void change_sign_checker( hash_iface *new_signer )
        {
            hash_checker_.reset(new_signer);
        }

        void change_sign_maker( hash_iface *new_signer )
        {
            hash_maker_.reset(new_signer);
        }

        void change_transformer( transformer_iface *new_transformer )
        {
            transformer_.reset(new_transformer);
        }

        void change_reverter( transformer_iface *new_reverter)
        {
            reverter_.reset(new_reverter);
        }

        void pop_message( )
        {
            queue_->messages( ).pop_front( );
        }

        void push_rpc_message(uint64_t slot_id, ll_unit_sptr mess)
        {
            if( rpc_queue_.queue_exists( slot_id ) )
                rpc_queue_.write_queue( slot_id, mess );
        }

        void push_rpc_message_all( ll_unit_sptr mess)
        {
            rpc_queue_.write_all( mess );
        }

        void call_rpc_method( uint64_t slot_id, const ll_unit_type &llu )
        {
            rpc_queue_.add_queue( slot_id );
            send_message( llu );
        }

        void call_rpc_method( const ll_unit_type &llu )
        {
            send_message( llu );
        }

        void wait_call_slot( uint64_t slot_id, uint32_t millisec)
        {
            wait_result_codes qwr = rpc_queue_.wait_queue( slot_id,
                             vtrc::chrono::milliseconds(millisec) );
            raise_wait_error( qwr );
        }

        void read_slot_for(uint64_t slot_id, ll_unit_sptr &mess,
                                             uint32_t millisec)
        {
            wait_result_codes qwr =
                    rpc_queue_.read(
                        slot_id, mess,
                        vtrc::chrono::milliseconds(millisec) );
            raise_wait_error( qwr );
        }

        void read_slot_for(uint64_t slot_id,
                        std::deque<ll_unit_sptr> &data_list, uint32_t millisec )
        {
            wait_result_codes qwr = rpc_queue_.read_queue(
                        slot_id, data_list,
                        vtrc::chrono::milliseconds(millisec)) ;
            raise_wait_error( qwr );
        }

        void close_slot( uint64_t slot_id )
        {
            rpc_queue_.erase_queue( slot_id );
        }

        void cancel_slot( uint64_t slot_id )
        {
            rpc_queue_.cancel( slot_id );
        }

        const vtrc_rpc_lowlevel::options &get_method_options(
                              const gpb::MethodDescriptor *method)
        {
            upgradable_lock lck(options_map_lock_);

            options_map_type::const_iterator f(options_map_.find(method));

            vtrc::shared_ptr<vtrc_rpc_lowlevel::options> result;

            if( f == options_map_.end( ) ) {

                const vtrc_rpc_lowlevel::rpc_options_type &serv (
                    method->service( )->options( )
                        .GetExtension( vtrc_rpc_lowlevel::service_options ));

                const vtrc_rpc_lowlevel::rpc_options_type &meth (
                    method->options( )
                        .GetExtension( vtrc_rpc_lowlevel::method_options));

                result = vtrc::make_shared<vtrc_rpc_lowlevel::options>
                                                                (serv.opt( ));
                if( meth.has_opt( ) )
                    utilities::merge_messages( *result, meth.opt( ) );

                upgrade_to_unique ulck( lck );
                options_map_.insert( std::make_pair( method, result ) );

            } else {
                result = f->second;
            }

            return *result;
        }

        void cancel_all_slots( bool erase )
        {
            if( erase )
                rpc_queue_.erase_all( );
            else
                rpc_queue_.cancel_all( );
        }

        void closure( common::rpc_controller_sptr controller,
                      lowlevel_unit_sptr llu )
        {
            ;;;
        }

        common::rpc_service_wrapper_sptr get_service(const std::string &name)
        {
            return parent_->get_service_by_name( name );
        }

        bool make_call_impl( lowlevel_unit_sptr llu )
        {

            protocol_layer::context_holder ch( parent_, llu.get( ) );

            common::rpc_service_wrapper_sptr
                    service(get_service(llu->call( ).service( )));

            if( !service->service( ) ) {
                throw vtrc::common::exception( vtrc_errors::ERR_BAD_FILE,
                                               "Service not found");
            }

            gpb::MethodDescriptor const *meth
                (service->get_method(llu->call( ).method( )));

            if( !meth ) {
                throw vtrc::common::exception( vtrc_errors::ERR_NO_FUNC );
            }

            const vtrc_rpc_lowlevel::options &call_opts
                                        ( parent_->get_method_options( meth ) );

            ch.ctx_->set_call_options( call_opts );

            vtrc::shared_ptr<gpb::Message> req
                (service->service( )->GetRequestPrototype( meth ).New( ));

            vtrc::shared_ptr<gpb::Message> res
                (service->service( )->GetResponsePrototype( meth ).New( ));

            req->ParseFromString( llu->request( ) );
            res->ParseFromString( llu->response( ));

            common::rpc_controller_sptr controller
                                (vtrc::make_shared<common::rpc_controller>( ));

            vtrc::shared_ptr<gpb::Closure> clos
                    (gpb::NewPermanentCallback( this, &this_type::closure,
                                                controller, llu ));

            service->service( )
                   ->CallMethod( meth, controller.get( ),
                                 req.get( ), res.get( ), clos.get( ) );

            if( controller->Failed( ) ) {
                throw vtrc::common::exception( vtrc_errors::ERR_INTERNAL,
                                               controller->ErrorText( ));
            } else if( controller->IsCanceled( ) ) {
                throw vtrc::common::exception( vtrc_errors::ERR_CANCELED );
            }

            llu->set_response( res->SerializeAsString( ) );
            return call_opts.wait( );
        }

        void make_call(protocol_layer::lowlevel_unit_sptr llu)
        {
            bool failed       = true;
            bool opt_wait     = true;
            bool request_wait = llu->opt( ).wait( );

            unsigned errorcode = 0;
            try {
                opt_wait = make_call_impl( llu );
                failed   = false;

            } catch ( const vtrc::common::exception &ex ) {
                errorcode = ex.code( );
                llu->mutable_error( )->set_additional( ex.additional( ) );

            } catch ( const std::exception &ex ) {
                errorcode = vtrc_errors::ERR_INTERNAL;
                llu->mutable_error( )->set_additional( ex.what( ) );

            } catch ( ... ) {
                errorcode = vtrc_errors::ERR_UNKNOWN;
                llu->mutable_error( )->set_additional( "..." );

            }

            if( opt_wait && request_wait ) {
                llu->clear_request( );
                llu->clear_call( );
                if( failed ) {
                    llu->mutable_error( )->set_code( errorcode );
                    llu->clear_response( );
                }
                send_message( *llu );
            } else {
                ;;;
            }
        }

        void on_system_error(const boost::system::error_code &err,
                             const std::string &add)
        {
            vtrc::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit>
                            llu( new  vtrc_rpc_lowlevel::lowlevel_unit );

            vtrc_errors::error_container *err_cont = llu->mutable_error( );

            err_cont->set_code(err.value( ));
            err_cont->set_category(vtrc_errors::CATEGORY_SYSTEM);
            err_cont->set_fatal( true );
            err_cont->set_additional( add );

            parent_->push_rpc_message_all( llu );

        }

    };

    protocol_layer::protocol_layer( transport_iface *connection )
        :impl_(new impl(connection))
    {
        impl_->parent_ = this;
    }

    protocol_layer::~protocol_layer( )
    {
        delete impl_;
    }

    void protocol_layer::process_data( const char *data, size_t length )
    {
        impl_->process_data( data, length );
    }

    std::string protocol_layer::prepare_data( const char *data, size_t length)
    {
        return impl_->prepare_data( data, length );
    }

    void protocol_layer::send_message(const google::protobuf::Message &message)
    {
        impl_->send_message( message );
    }

    call_context *protocol_layer::reset_call_context(call_context *cc)
    {
        return impl_->reset_call_context( cc );
    }

    call_context *protocol_layer::release_call_context( )
    {
        return impl_->release_call_context( );
    }

    const vtrc_rpc_lowlevel::options &protocol_layer::get_method_options(
                              const gpb::MethodDescriptor *method)
    {
        return impl_->get_method_options( method );
    }


    const call_context *protocol_layer::get_call_context( ) const
    {
        return impl_->get_call_context( );
    }

    call_context *protocol_layer::mutable_call_context( )
    {
        return impl_->get_call_context( );
    }

    bool protocol_layer::check_message( const std::string &mess )
    {
        return impl_->check_message( mess );
    }

    void protocol_layer::parse_message( const std::string &mess,
                                        google::protobuf::Message &result )
    {
        impl_->parse_message(mess, result);
    }

    void protocol_layer::make_call(protocol_layer::lowlevel_unit_sptr llu)
    {
        impl_->make_call( llu );
    }

    message_queue_type &protocol_layer::message_queue( )
    {
        return impl_->message_queue( );
    }

    const message_queue_type &protocol_layer::message_queue( ) const
    {
        return impl_->message_queue( );
    }

    void protocol_layer::change_hash_maker( hash_iface *new_hasher )
    {
        impl_->change_sign_maker( new_hasher );
    }

    void protocol_layer::change_hash_checker( hash_iface *new_hasher )
    {
        impl_->change_sign_checker( new_hasher );
    }

    void protocol_layer::change_transformer( transformer_iface *new_transformer)
    {
        impl_->change_transformer( new_transformer );
    }

    void protocol_layer::change_reverter( transformer_iface *new_reverter)
    {
        impl_->change_reverter( new_reverter );
    }

    void protocol_layer::pop_message( )
    {
        impl_->pop_message( );
    }

    uint64_t protocol_layer::next_index( )
    {
        return impl_->next_index( );
    }

    void protocol_layer::call_rpc_method( const ll_unit_type &llu )
    {
        impl_->call_rpc_method( llu );
    }

    void protocol_layer::call_rpc_method( uint64_t slot_id,
                                          const ll_unit_type &llu )
    {
        impl_->call_rpc_method( slot_id, llu );
    }

    void protocol_layer::wait_slot_for( uint64_t slot_id, uint32_t millisec)
    {
        impl_->wait_call_slot( slot_id, millisec);
    }

    void protocol_layer::read_slot_for( uint64_t slot_id,
                                        ll_unit_sptr &mess,
                                        uint32_t millisec)
    {
        impl_->read_slot_for( slot_id, mess, millisec);
    }

    void protocol_layer::read_slot_for( uint64_t slot_id,
                                        std::deque<ll_unit_sptr> &mess_list,
                                        uint32_t millisec )
    {
        impl_->read_slot_for( slot_id, mess_list, millisec);
    }

    void protocol_layer::erase_slot(uint64_t slot_id)
    {
        impl_->close_slot( slot_id );
    }

    void protocol_layer::cancel_slot(uint64_t slot_id)
    {
        impl_->cancel_slot( slot_id );
    }

    void protocol_layer::cancel_all_slots( )
    {
        impl_->cancel_all_slots( false );
    }

    void protocol_layer::erase_all_slots( )
    {
        impl_->cancel_all_slots( true );
    }

    void protocol_layer::push_rpc_message(uint64_t slot_id, ll_unit_sptr mess)
    {
        impl_->push_rpc_message(slot_id, mess);
    }

    void protocol_layer::push_rpc_message_all( ll_unit_sptr mess)
    {
        impl_->push_rpc_message_all( mess );
    }

    void protocol_layer::on_write_error(const boost::system::error_code &err)
    {
        impl_->on_system_error( err, "Transport write error." );
    }

    void protocol_layer::on_read_error(const boost::system::error_code &err)
    {
        impl_->on_system_error( err, "Transport read error." );
    }

}}
