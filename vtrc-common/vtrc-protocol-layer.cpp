
#include "boost/thread/tss.hpp"

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

#include <exception>

#include "vtrc-thread.h"

#include "vtrc-atomic.h"
#include "vtrc-mutex.h"
#include "vtrc-memory.h"
#include "vtrc-mutex-typedefs.h"
#include "vtrc-condition-variable.h"

#include "vtrc-protocol-layer.h"

#include "vtrc-data-queue.h"
#include "vtrc-hash-iface.h"
#include "vtrc-transformer-iface.h"

#include "vtrc-transport-iface.h"

#include "vtrc-condition-queues.h"
#include "vtrc-exception.h"
#include "vtrc-call-context.h"

#include "vtrc-rpc-controller.h"
#include "vtrc-random-device.h"

#include "proto-helper/message-utilities.h"

#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "protocol/vtrc-rpc-options.pb.h"
#include "protocol/vtrc-errors.pb.h"

namespace vtrc { namespace common {

    namespace gpb   = google::protobuf;
    namespace bsys  = boost::system;

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

        static const size_t default_max_message_length = 1024 * 1024;

        struct rpc_unit_index {
            uint64_t     id_;
            rpc_unit_index( uint64_t id )
                :id_(id)
            { }
        };

        inline bool operator < ( const rpc_unit_index &lhs,
                                 const rpc_unit_index &rhs )
        {
            return lhs.id_ < rhs.id_;
        }

        typedef vtrc_rpc::lowlevel_unit              lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        typedef condition_queues <
                uint64_t,
                lowlevel_unit_sptr
        > rpc_queue_type;


        typedef std::map <
             const google::protobuf::MethodDescriptor *
            ,vtrc::shared_ptr<vtrc_rpc::options>
        > options_map_type;

        struct closure_holder_type {

            connection_iface_wptr                   connection_;
            vtrc::scoped_ptr<gpb::Message>          req_;
            vtrc::scoped_ptr<gpb::Message>          res_;
            vtrc::scoped_ptr<rpc_controller>        controller_;
            lowlevel_unit_sptr                      llu_;
            common::closure_type                    internal_closure_;
            gpb::Closure                           *proto_closure_;
            bool                                    called_;

            closure_holder_type( )
                :proto_closure_(NULL)
                ,called_(false)
            { }

            ~closure_holder_type( ) try
            {
                if( proto_closure_ )
                    delete proto_closure_;

                if( internal_closure_ ) {
                    const bsys::error_code error_success(0,
                                            bsys::get_system_category( ));
                    internal_closure_( error_success );
                }

            } catch( ... ) { ;;; }

        };

        typedef vtrc::shared_ptr<closure_holder_type> closure_holder_sptr;

        namespace size_policy_ns = data_queue::varint;

    }

    struct protocol_layer::impl {

        typedef impl this_type;
        typedef protocol_layer parent_type;
        typedef parent_type::call_stack_type                 call_stack_type;
        typedef boost::thread_specific_ptr<call_stack_type>  call_context_ptr;

        transport_iface             *connection_;
        protocol_layer              *parent_;

        vtrc::scoped_ptr<hash_iface>             hash_maker_;
        vtrc::scoped_ptr<hash_iface>             hash_checker_;
        vtrc::scoped_ptr<transformer_iface>      transformer_;
        vtrc::scoped_ptr<transformer_iface>      revertor_;

        vtrc::scoped_ptr<data_queue::queue_base> queue_;

        rpc_queue_type               rpc_queue_;
        vtrc::atomic<uint64_t>       rpc_index_;

        call_context_ptr             context_;

        options_map_type             options_map_;
        mutable vtrc::shared_mutex   options_map_lock_;

        const lowlevel_unit_type     empty_done_;

        mutable vtrc::mutex          ready_lock_;
        bool                         ready_;
        vtrc::condition_variable     ready_var_;

        unsigned                     level_;

        impl( transport_iface *c, bool oddside, size_t mess_len )
            :connection_(c)
            ,hash_maker_(common::hash::create_default( ))
            ,hash_checker_(common::hash::create_default( ))
            ,transformer_(common::transformers::none::create( ))
            ,revertor_(common::transformers::none::create( ))
            ,queue_(size_policy_ns::create_parser(mess_len))
            ,rpc_index_(oddside ? 101 : 100)
            ,empty_done_(make_fake_mess( ))
            ,ready_(false)
            ,level_(0)
        { }

        ~impl( )
        { }

        static protocol_layer::lowlevel_unit_type make_fake_mess( )
        {

            random_device rd(true);
            std::string s1( rd.generate_block( 4 ) );
            //std::string s2( rd.generate_block( 4 ) );
            protocol_layer::lowlevel_unit_type res;

            res.set_request( s1 );
            //res.set_response( s2 );

            return res;
        }

        std::string prepare_data( const char *data, size_t length )
        {
            /**
             * message_header = <packed_size(data_length + hash_length)>
            **/
            std::string result(size_policy_ns::pack_size(
                                length + hash_maker_->hash_size( )));

            /** here is:
             *  message_body = <hash(data)> + <data>
            **/
            std::string body( hash_maker_->get_data_hash(data, length ) );
            body.append( data, data + length );

            /**
             * message =  message_header + <transform( message )>
            **/
            transformer_->transform( body.empty( ) ? NULL : &body[0],
                                     body.size( ) );

            result.append( body.begin( ), body.end( ) );

            return result;
        }

        void process_data( const char *data, size_t length )
        {
            if( length > 0 ) {
                std::string next_data(data, data + length);

                const size_t old_size = queue_->messages( ).size( );

                /**
                 * message = <size>transformed(data)
                 * we must revert data in 'parse_and_pop_top'
                **/
                queue_->append( &next_data[0], next_data.size( ));
                queue_->process( );

                if( queue_->messages( ).size( ) > old_size ) {
                    parent_->on_data_ready( );
                }
            }

        }

        bool parse_and_pop( gpb::MessageLite &result )
        {
            std::string &data(queue_->messages( ).front( ));

            /// revert message
            revertor_->transform( data.empty( ) ? NULL : &data[0],
                                  data.size( ) );
            /// check hash
            bool checked = check_message( data );
            if( checked ) {
                /// parse
                checked = parse_message( data, result );
            }

            /// in all cases we pop message
            queue_->messages( ).pop_front( );
            return checked;
        }

        size_t ready_messages_count( ) const
        {
            return queue_->messages( ).size( );
        }

        bool message_queue_empty( ) const
        {
            return queue_->messages( ).empty( );
        }

        bool parse_message( const std::string &mess, gpb::MessageLite &result )
        {
            const size_t hash_length = hash_checker_->hash_size( );
            return result.ParseFromArray( mess.c_str( ) + hash_length,
                                          mess.size( )  - hash_length );
        }

        void set_ready( bool ready )
        {
            vtrc::unique_lock<vtrc::mutex> lck( ready_lock_ );
            ready_ = ready;
            ready_var_.notify_all( );
        }

        bool state_predic( bool state ) const
        {
            return (ready_ == state);
        }

        bool ready( ) const
        {
            return ready_;
        }

        void wait_for_ready( bool state )
        {
            vtrc::unique_lock<vtrc::mutex> lck( ready_lock_ );
            if( state != ready_ )
                ready_var_.wait( lck,
                         vtrc::bind( &this_type::state_predic, this, state ));
        }

        template <typename DurationType>
        bool wait_for_ready_for( bool state, const DurationType &duration )
        {
            vtrc::unique_lock<vtrc::mutex> lck( ready_lock_ );
            return ready_var_.wait_for( lck, duration,
                     vtrc::bind( &this_type::state_predic, this, state ) );
        }

        void drop_first( )
        {
            queue_->messages( ).pop_front( );
        }

        void send_data( const char *data, size_t length )
        {
            connection_->write( data, length );
        }

        void send_message( const gpb::MessageLite &message )
        {
            std::string ser(message.SerializeAsString( ));
            send_data( ser.c_str( ), ser.size( ) );
        }

        uint64_t next_index( )
        {
            return (rpc_index_ += 2);
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

        void check_create_stack( )
        {
            if( NULL == context_.get( ) ) {
                context_.reset( new call_stack_type );
            }
        }

        call_context *push_call_context(vtrc::shared_ptr<call_context> cc)
        {
            check_create_stack( );
            call_context *top = top_call_context( );
            cc->set_next( top );
            context_->push_front( cc );
            return context_->front( ).get( );
        }

        call_context *push_call_context( call_context *cc)
        {
            return push_call_context( vtrc::shared_ptr<call_context>(cc) );
        }

        void pop_call_context( )
        {
            if( !context_->empty( ) )
                context_->pop_front( );
        }

        void reset_call_context( )
        {
            context_.reset(  );
        }

        void swap_call_stack( call_stack_type &other )
        {
            check_create_stack( );
            std::swap( *context_, other );
        }

        static vtrc::shared_ptr<call_context> clone_context( call_context &src )
        {
            return vtrc::make_shared<call_context>( src );
        }

        static void clone_stack( call_stack_type &src, call_stack_type &res )
        {
            typedef call_stack_type::const_iterator iter;

            call_stack_type tmp;
            call_context *last(NULL);

            for( iter b(src.begin( )), e(src.end( )); b!=e; ++b ) {
                tmp.push_back( clone_context( *(*b) ) );
                if( last ) {
                    last->set_next( b->get( ) );
                }
                last = b->get( );
            }
            res.swap( tmp );
        }

        void copy_call_stack( call_stack_type &other ) const
        {
            if( NULL == context_.get( ) ) {
                other.clear( );
            } else {
                clone_stack( *context_, other );
            }
        }

        const call_context *get_call_context( ) const
        {
            return context_.get( ) && !context_->empty( )
                    ? context_->front( ).get( )
                    : NULL;
        }

        call_context *top_call_context( )
        {
            return context_.get( ) && !context_->empty( )
                    ? context_->front( ).get( )
                    : NULL;
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

        void change_revertor( transformer_iface *new_reverter)
        {
            revertor_.reset(new_reverter);
        }

        void push_rpc_message(uint64_t slot_id, lowlevel_unit_sptr mess)
        {
            rpc_queue_.write_queue_if_exists( slot_id, mess );
        }

        void push_rpc_message_all( lowlevel_unit_sptr mess)
        {
            rpc_queue_.write_all( mess );
        }

        void call_rpc_method( uint64_t slot_id, const lowlevel_unit_type &llu )
        {
            if( !ready_ )
                throw vtrc::common::exception( vtrc_errors::ERR_COMM );
            rpc_queue_.add_queue( slot_id );
            send_message( llu );
        }

        void call_rpc_method( const lowlevel_unit_type &llu )
        {
            if( !ready_ )
                throw vtrc::common::exception( vtrc_errors::ERR_COMM );
            send_message( llu );
        }

        void wait_call_slot( uint64_t slot_id, uint64_t microsec)
        {
            wait_result_codes qwr = rpc_queue_.wait_queue( slot_id,
                             vtrc::chrono::microseconds(microsec) );
            raise_wait_error( qwr );
        }

        void read_slot_for(uint64_t slot_id, lowlevel_unit_sptr &mess,
                                             uint64_t microsec)
        {
            wait_result_codes qwr =
                    rpc_queue_.read(
                        slot_id, mess,
                        vtrc::chrono::microseconds(microsec) );

            raise_wait_error( qwr );
        }

        void read_slot_for(uint64_t slot_id,
                           std::deque<lowlevel_unit_sptr> &data_list,
                           uint64_t microsec )
        {
            wait_result_codes qwr = rpc_queue_.read_queue( slot_id, data_list,
                                         vtrc::chrono::microseconds(microsec));
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

        void set_level( unsigned level )
        {
            level_ = level;
        }

        unsigned get_level( ) const
        {
            return level_;
        }

        const vtrc_rpc::options &get_method_options(
                              const gpb::MethodDescriptor *method)
        {
            upgradable_lock lck(options_map_lock_);

            options_map_type::const_iterator f(options_map_.find(method));

            vtrc::shared_ptr<vtrc_rpc::options> result;

            if( f == options_map_.end( ) ) {

                const vtrc_rpc::options &serv (
                        method->service( )->options( )
                            .GetExtension( vtrc_rpc::service_options ));

                const vtrc_rpc::options &meth (
                        method->options( )
                            .GetExtension( vtrc_rpc::method_options ));

                result = vtrc::make_shared<vtrc_rpc::options>( serv );
                utilities::merge_messages( *result, meth );

                upgrade_to_unique ulck( lck );
                options_map_.insert( std::make_pair( method, result ) );

            } else {
                result = f->second;
            }

            return *result;
        }

        void erase_all_slots(  )
        {
            rpc_queue_.erase_all( );
        }

        void cancel_all_slots( )
        {
            rpc_queue_.cancel_all( );
        }

        common::rpc_service_wrapper_sptr get_service(const std::string &name)
        {
            return parent_->get_service_by_name( name );
        }

        void closure_fake( closure_holder_sptr /*holder*/ )
        {
            //send_message( fake_ );
        }

        void closure_done( closure_holder_sptr holder )
        {
            lowlevel_unit_sptr &llu = holder->llu_;

            bool failed        = false;
            unsigned errorcode = 0;

            if( holder->controller_->IsCanceled( ) ) {

                errorcode = vtrc_errors::ERR_CANCELED;
                failed = true;

             } else if( holder->controller_->Failed( ) ) {

                errorcode = vtrc_errors::ERR_INTERNAL;
                failed = true;

            }

            llu->clear_request( );
            llu->clear_call( );

            if( failed ) {

                llu->mutable_error( )->set_code( errorcode );
                llu->clear_response( );

                if( !holder->controller_->ErrorText( ).empty( ) )
                    llu->mutable_error( )
                          ->set_additional( holder->controller_->ErrorText( ));

            } else {
                llu->set_response( holder->res_->SerializeAsString( ) );
            }
            send_message( *llu );
        }

        void closure_runner( closure_holder_sptr holder, bool wait )
        {
            if( !holder->called_ ) {
                holder->called_ = true;
            } else {
                return;
            }

            if( std::uncaught_exception( ) ) {
//                std::cerr << "Uncaught exception at done handler for "
//                          << holder->llu_->call( ).service_id( )
//                          << "::"
//                          << holder->llu_->call( ).method_id( )
//                          << std::endl;
            } else {
                connection_iface_sptr lck(holder->connection_.lock( ));
                if( lck )
                    wait ? closure_done( holder ) : closure_fake( holder );
            }
        }

        gpb::Closure *make_closure(closure_holder_sptr &closure_hold, bool wait)
        {
            closure_hold->proto_closure_ =
                    gpb::NewPermanentCallback( this, &this_type::closure_runner,
                                               closure_hold, wait );
            return closure_hold->proto_closure_;
        }

        void make_call_impl( lowlevel_unit_sptr llu, closure_type done )
        {

            protocol_layer::context_holder ch( parent_, llu.get( ) );

            common::rpc_service_wrapper_sptr
                    service(get_service(llu->call( ).service_id( )));

            if( !service || !service->service( ) ) {
                throw vtrc::common::exception( vtrc_errors::ERR_BAD_FILE,
                                               "Service not found");
            }

            gpb::MethodDescriptor const *meth
                (service->get_method(llu->call( ).method_id( )));

            if( !meth ) {
                throw vtrc::common::exception( vtrc_errors::ERR_NO_FUNC );
            }

            const vtrc_rpc::options &call_opts
                                        ( parent_->get_method_options( meth ) );

            ch.ctx_->set_call_options( call_opts );

            closure_holder_sptr closure_hold
                                (vtrc::make_shared<closure_holder_type>( ));

            closure_hold->req_.reset
                (service->service( )->GetRequestPrototype( meth ).New( ));

            closure_hold->res_.reset
                (service->service( )->GetResponsePrototype( meth ).New( ));

            closure_hold->req_->ParseFromString( llu->request( ) );
            closure_hold->res_->ParseFromString( llu->response( ));

            closure_hold->controller_.reset(new common::rpc_controller);

            closure_hold->      connection_ = connection_->weak_from_this( );
            closure_hold->             llu_ = llu;
            closure_hold->internal_closure_ = done;

            gpb::Closure* clos(make_closure(closure_hold, llu->opt( ).wait( )));
            ch.ctx_->set_done_closure( clos );

            service->service( )->CallMethod( meth,
                                     closure_hold->controller_.get( ),
                                     closure_hold->req_.get( ),
                                     closure_hold->res_.get( ), clos );
        }

        void make_call(protocol_layer::lowlevel_unit_sptr llu)
        {
            make_call(llu, closure_type( ));
        }

        void make_call(protocol_layer::lowlevel_unit_sptr llu, closure_type don)
        {
            bool failed        = true;
            unsigned errorcode = 0;

            try {

                make_call_impl( llu, don );
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

            if( llu->opt( ).wait( ) ) {
                if( failed ) {
                    llu->mutable_error( )->set_code( errorcode );
                    llu->clear_response( );
                    send_message( *llu );
                }
            } else {
                send_message( empty_done_ );
            }

        }

        void on_system_error(const boost::system::error_code &err,
                             const std::string &add)
        {
            set_ready( false );

            vtrc::shared_ptr<vtrc_rpc::lowlevel_unit>
                            llu( new  vtrc_rpc::lowlevel_unit );

            vtrc_errors::container *err_cont = llu->mutable_error( );

            err_cont->set_code(err.value( ));
            err_cont->set_category(vtrc_errors::CATEGORY_SYSTEM);
            err_cont->set_fatal( true );
            err_cont->set_additional( add );

            parent_->push_rpc_message_all( llu );
        }

    };

    protocol_layer::protocol_layer( transport_iface *connection, bool oddside )
        :impl_(new impl(connection, oddside, default_max_message_length))
    {
        impl_->parent_ = this;
    }

     protocol_layer::protocol_layer(transport_iface *connection,
                                    bool oddside, size_t maximum_mess_len)
         :impl_(new impl(connection, oddside, maximum_mess_len))
     {
         impl_->parent_ = this;
     }

    protocol_layer::~protocol_layer( )
    {
        delete impl_;
    }

    bool protocol_layer::ready( ) const
    {
        return impl_->ready( );
    }

    void protocol_layer::wait_for_ready( bool state )
    {
        return impl_->wait_for_ready( state );
    }

    bool protocol_layer::wait_for_ready_for_millisec(bool ready,
                                                     uint64_t millisec) const
    {
        return impl_->wait_for_ready_for( ready,
                vtrc::chrono::milliseconds(millisec));
    }

    bool protocol_layer::wait_for_ready_for_microsec(bool ready,
                                                     uint64_t microsec) const
    {
        return impl_->wait_for_ready_for( ready,
                vtrc::chrono::microseconds(microsec));
    }

    void protocol_layer::set_ready( bool ready )
    {
        impl_->set_ready( ready );
    }

    void protocol_layer::process_data( const char *data, size_t length )
    {
        impl_->process_data( data, length );
    }

    std::string protocol_layer::prepare_data( const char *data, size_t length)
    {
        return impl_->prepare_data( data, length );
    }


    /*
     * call context
    */

    call_context *protocol_layer::push_call_context(
                                             vtrc::shared_ptr<call_context> cc)
    {
        return impl_->push_call_context( cc );
    }

    call_context *protocol_layer::push_call_context( call_context *cc )
    {
        return impl_->push_call_context( cc );
    }

    void protocol_layer::pop_call_context( )
    {
        return impl_->pop_call_context( );
    }

    void protocol_layer::reset_call_stack( )
    {
        impl_->reset_call_context(  );
    }

    void protocol_layer::swap_call_stack(protocol_layer::call_stack_type &other)
    {
        impl_->swap_call_stack( other );
    }

    void protocol_layer::copy_call_stack(
                                protocol_layer::call_stack_type &other) const
    {
        impl_->copy_call_stack( other );
    }

    const vtrc_rpc::options &protocol_layer::get_method_options(
                              const gpb::MethodDescriptor *method)
    {
        return impl_->get_method_options( method );
    }

    void protocol_layer::set_level( unsigned level )
    {
        impl_->set_level( level );
    }

    unsigned protocol_layer::get_level( ) const
    {
        return impl_->get_level( );
    }

    const call_context *protocol_layer::get_call_context( ) const
    {
        return impl_->get_call_context( );
    }

    call_context *protocol_layer::top_call_context( )
    {
        return impl_->top_call_context( );
    }

    // ===============

    void protocol_layer::make_call(protocol_layer::lowlevel_unit_sptr llu)
    {
        impl_->make_call( llu );
    }

    void protocol_layer::make_call(protocol_layer::lowlevel_unit_sptr llu,
                                   closure_type done)
    {
        impl_->make_call( llu, done );
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

    void protocol_layer::change_revertor( transformer_iface *new_reverter)
    {
        impl_->change_revertor( new_reverter );
    }

    size_t protocol_layer::ready_messages_count( ) const
    {
        return impl_->ready_messages_count( );
    }

    bool protocol_layer::message_queue_empty( ) const
    {
        return impl_->message_queue_empty( );
    }

    bool protocol_layer::parse_and_pop( gpb::MessageLite &result )
    {
        return impl_->parse_and_pop( result );
    }

    uint64_t protocol_layer::next_index( )
    {
        return impl_->next_index( );
    }

    void protocol_layer::call_rpc_method( const lowlevel_unit_type &llu )
    {
        impl_->call_rpc_method( llu );
    }

    void protocol_layer::call_rpc_method( uint64_t slot_id,
                                          const lowlevel_unit_type &llu )
    {
        impl_->call_rpc_method( slot_id, llu );
    }

    void protocol_layer::wait_slot_for(uint64_t slot_id, uint64_t microsec )
    {
        impl_->wait_call_slot( slot_id, microsec );
    }

    void protocol_layer::read_slot_for(uint64_t slot_id,
                                        lowlevel_unit_sptr &mess,
                                        uint64_t microsec)
    {
        impl_->read_slot_for( slot_id, mess, microsec );
    }

    void protocol_layer::read_slot_for( uint64_t slot_id,
                                      std::deque<lowlevel_unit_sptr> &mess_list,
                                      uint64_t microsec )
    {
        impl_->read_slot_for( slot_id, mess_list, microsec );
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
        impl_->cancel_all_slots( );
    }

//    void protocol_layer::close_queue( )
//    {
//        impl_->close_queue( );
//    }

    void protocol_layer::erase_all_slots( )
    {
        impl_->erase_all_slots( );
    }

    void protocol_layer::push_rpc_message(uint64_t slot_id,
                                          lowlevel_unit_sptr mess)
    {
        impl_->push_rpc_message(slot_id, mess);
    }

    void protocol_layer::push_rpc_message_all( lowlevel_unit_sptr mess)
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
