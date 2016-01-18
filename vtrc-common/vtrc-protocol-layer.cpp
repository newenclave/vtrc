
#include "boost/thread/tss.hpp"
#include "boost/asio.hpp"

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
#include "vtrc-protocol-defaults.h"

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

#include "vtrc-rpc-lowlevel.pb.h"
#include "vtrc-rpc-options.pb.h"
#include "vtrc-errors.pb.h"

namespace vtrc { namespace common {

    namespace gpb   = google::protobuf;
    namespace bsys  = boost::system;
    namespace basio = boost::asio;

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
                raise_error( rpc::errors::ERR_CANCELED );
                break;
            case  WAIT_RESULT_TIMEOUT:
                raise_error( rpc::errors::ERR_TIMEOUT );
                break;
            default:
                break;
            }
        }
#if 0
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
#endif
        typedef rpc::lowlevel_unit                   lowlevel_unit_type;
        typedef vtrc::shared_ptr<lowlevel_unit_type> lowlevel_unit_sptr;

        typedef condition_queues <
                uint64_t,
                lowlevel_unit_sptr
        > rpc_queue_type;


        typedef std::map <
             const google::protobuf::MethodDescriptor *
            ,vtrc::shared_ptr<rpc::options>
        > options_map_type;

        struct closure_holder_type {

            connection_iface_wptr                   connection_;
            vtrc::unique_ptr<gpb::Message>          req_;
            vtrc::unique_ptr<gpb::Message>          res_;
            vtrc::unique_ptr<rpc_controller>        controller_;
            lowlevel_unit_sptr                      llu_;
            vtrc::unique_ptr<protcol_closure_type>  internal_closure_;
            gpb::Closure                           *proto_closure_;

            closure_holder_type( )
                :proto_closure_(NULL)
            { }

            void call_internal( )
            {
                if( NULL != internal_closure_.get( ) && (*internal_closure_)) {
                    connection_iface_sptr lck(connection_.lock( ));
                    if( lck  )  {
                        (*internal_closure_)( llu_->error( ) );
                    }
                }
            }

            ~closure_holder_type( )
            {
                try {
                    if( proto_closure_ ) {
                        delete proto_closure_;
                    }

                    call_internal( );

                } catch( ... ) { ;;; }
            }

        };

        typedef vtrc::shared_ptr<closure_holder_type> closure_holder_sptr;

        namespace size_policy_ns = data_queue::varint;

        typedef std::deque< vtrc::shared_ptr<call_context> > call_stack_type;

#if VTRC_DISABLE_CXX11
        /// call context ptr BOOST
        typedef std::deque< vtrc::shared_ptr<call_context> > call_stack_type;
        typedef boost::thread_specific_ptr<call_stack_type>  call_context_ptr;
        static call_context_ptr s_call_context;
#else
        //// NON BOOST
        typedef call_stack_type *call_context_ptr;
        static VTRC_THREAD_LOCAL call_context_ptr s_call_context = nullptr;

#endif  /// VTRC_DISABLE_CXX11

        precall_closure_type empty_pre( )
        {
            struct call_type {
                static void call( connection_iface &,
                                  const gpb::MethodDescriptor *,
                                  rpc::lowlevel_unit & )
                { }
            };
            namespace ph = vtrc::placeholders;
            return vtrc::bind( &call_type::call, ph::_1, ph::_2, ph::_3 );
        }

        postcall_closure_type empty_post( )
        {
            struct call_type {
                static void call( connection_iface &,
                                  rpc::lowlevel_unit & )
                { }
            };
            namespace ph = vtrc::placeholders;
            return vtrc::bind( &call_type::call, ph::_1, ph::_2 );
        }

    }

    struct protocol_layer::impl {

        typedef impl this_type;
        typedef protocol_layer parent_type;
        typedef parent_type::call_stack_type                 call_stack_type;
        typedef boost::thread_specific_ptr<call_stack_type>  call_context_ptr;

        transport_iface             *connection_;
        protocol_layer              *parent_;

        vtrc::unique_ptr<hash_iface>             hash_maker_;
        vtrc::unique_ptr<hash_iface>             hash_checker_;
        vtrc::unique_ptr<transformer_iface>      transformer_;
        vtrc::unique_ptr<transformer_iface>      revertor_;
        vtrc::unique_ptr<data_queue::queue_base> queue_;

        rpc_queue_type               rpc_queue_;
        vtrc::atomic<uint64_t>       rpc_index_;

        options_map_type             options_map_;
        mutable vtrc::shared_mutex   options_map_lock_;

        mutable vtrc::mutex          ready_lock_;
        bool                         ready_;
        vtrc::condition_variable     ready_var_;

        unsigned                     level_;

        rpc::session_options         session_opts_;

        precall_closure_type         precall_;
        postcall_closure_type        postcall_;

        impl( transport_iface *c, bool odd, const rpc::session_options &opts )
            :connection_(c)
            ,hash_maker_(common::hash::create_default( ))
            ,hash_checker_(common::hash::create_default( ))
            ,transformer_(common::transformers::none::create( ))
            ,revertor_(common::transformers::none::create( ))
            ,queue_(size_policy_ns::create_parser(opts.max_message_length( )))
            ,rpc_index_(odd ? 101 : 100)
            ,ready_(false)
            ,level_(0)
            ,session_opts_(opts)
            ,precall_(empty_pre( ))
            ,postcall_(empty_post( ))
        { }

        /// call_stack pointer BOOST ///
#if VTRC_DISABLE_CXX11
        static call_stack_type *context_get( )
        {
            return s_call_context.get( );
        }

        static void context_reset( call_stack_type *new_inst = NULL )
        {
            s_call_context.reset( new_inst );
        }
#else
        static call_stack_type *context_get( )
        {
            return s_call_context;
        }

        static void context_reset( call_stack_type *new_inst = NULL )
        {
            if( s_call_context ) {
                delete s_call_context;
            }
            s_call_context = new_inst;
        }
#endif
        //////////////////////////

        ~impl( )
        { }

        void configure_session( const rpc::session_options &opts )
        {
            session_opts_.CopyFrom( opts );
            queue_->set_maximum_length( opts.max_message_length( ) );
        }

#define VTRC_PROTOCOL_PACK_SIZE_DEFAULT 1

#if VTRC_PROTOCOL_PACK_SIZE_DEFAULT /// variant uno
        std::string prepare_data( const char *data, size_t length )
        {
            /**
             * message_header = <packed_size(data_length + hash_length)>
            **/
            const size_t body_len = length + hash_maker_->hash_size( );
            std::string result( size_policy_ns::pack_size( body_len ));

            /** here is:
             *  message_body = <hash(data)> + <data>
            **/
            std::string body( hash_maker_->get_data_hash( data, length ) );
            body.append( data, data + length );

            /**
             * message =  message_header + <transform( message )>
            **/
            transformer_->transform( body );
//            transformer_->transform( body.empty( ) ? NULL : &body[0],
//                                     body.size( ) );

            result.append( body.begin( ), body.end( ) );

            return result;
        }


        void process_data( const char *data, size_t length )
        {
            if( length > 0 ) {

                //std::string next_data(data, data + length);

                const size_t old_size = queue_->messages( ).size( );

                /**
                 * message = <size>transformed(data)
                 * we must revert data in 'parse_and_pop'
                **/

                //queue_->append( &next_data[0], next_data.size( ));

                queue_->append( data, length );
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
            revertor_->transform( data );
//            revertor_->transform( data.empty( ) ? NULL : &data[0],
//                                  data.size( ) );
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

        bool raw_pop( std::string &result )
        {
            std::string &data( queue_->messages( ).front( ) );

            /// revert message
            revertor_->transform( data );
//            revertor_->transform( data.empty( ) ? NULL : &data[0],
//                                  data.size( ) );

            /// check hash
            bool checked = check_message( data );
            if( checked ) {
                const size_t hash_length = hash_checker_->hash_size( );
                result.assign( data.c_str( ) + hash_length,
                               data.size( )  - hash_length );
            }

            /// in all cases we pop message
            queue_->messages( ).pop_front( );

            return checked;
        }

#else
        std::string prepare_data( const char *data, size_t length )
        {
            /**
             * message_header = <packed_size(data_length + hash_length)>
            **/
            const size_t body_len = length + hash_maker_->hash_size( );

            std::string body( body_len + 32, '?' );

            /**
             *  Pack length of the data and the hash to vector
             *  Get size of packed size value;
            **/
            size_t siz_len = size_policy_ns::pack_size_to( body_len, &body[0] );

            /**
             *  here is:
             *  message_body = <hash(data)> + <data>
            **/
            hash_maker_->get_data_hash( data, length, &body[siz_len] );

            if( length > 0 ) {
                memcpy( &body[hash_maker_->hash_size( ) + siz_len],
                         data, length );
            }

            /**
             *  transform( <size><hash><message> )
            **/
            body.resize( body_len + siz_len );

            transformer_->transform( body );

            return body;
        }

        void process_data( const char *data, size_t length )
        {
            if( length > 0 ) {

                std::string next_data(data, data + length);

                const size_t old_size = queue_->messages( ).size( );

                /// revert data block
                revertor_->transform( next_data );

                /**
                 * message = transformed(<size>data)
                 * we must revert data here
                **/

                //queue_->append( &next_data[0], next_data.size( ));

                queue_->append( next_data.empty( ) ? NULL : &next_data[0],
                                next_data.size( ) );
                queue_->process( );

                if( queue_->messages( ).size( ) > old_size ) {
                    parent_->on_data_ready( );
                }
            }
        }

        bool raw_pop( std::string &result )
        {
            /// doesntwork
            std::string &data( queue_->messages( ).front( ) );

            /// revert message
            revertor_->transform( data );
//            revertor_->transform( data.empty( ) ? NULL : &data[0],
//                                  data.size( ) );

            /// check hash
            bool checked = check_message( data );
            if( checked ) {
                const size_t hash_length = hash_checker_->hash_size( );
                result.assign( data.c_str( ) + hash_length,
                               data.size( )  - hash_length );
            }

            /// in all cases we pop message
            queue_->messages( ).pop_front( );

            return checked;
        }

        bool parse_and_pop( gpb::MessageLite &result )
        {
            std::string &data(queue_->messages( ).front( ));

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

#endif
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
            return ( rpc_index_ += 2 );
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
            if( NULL == context_get( ) ) {
                //std::cout << "Stack is empty!\n";
                context_reset( new call_stack_type );
            }
        }

        call_context *push_call_context(vtrc::shared_ptr<call_context> cc)
        {
            check_create_stack( );
            call_context *top = top_call_context( );
            cc->set_next( top );
            context_get( )->push_front( cc );
            return context_get( )->front( ).get( );
        }

        call_context *push_call_context( call_context *cc)
        {
            return push_call_context( vtrc::shared_ptr<call_context>(cc) );
        }

        void pop_call_context( )
        {
            if( !context_get( )->empty( ) ) {
                context_get( )->pop_front( );
            }

            if( context_get( )->empty( ) ) {
                context_reset( );
            }
        }

        void reset_call_context( )
        {
            context_reset(  );
        }

        void swap_call_stack( call_stack_type &other )
        {
            check_create_stack( );
            std::swap( *context_get( ), other );
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
            if( NULL == context_get( ) ) {
                other.clear( );
            } else {
                clone_stack( *context_get( ), other );
            }
        }

        const call_context *get_call_context( ) const
        {
            return context_get( ) && !context_get( )->empty( )
                    ? context_get( )->front( ).get( )
                    : NULL;
        }

        call_context *top_call_context( )
        {
            return (context_get( ) && !context_get( )->empty( ))
                    ? context_get( )->front( ).get( )
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
                throw vtrc::common::exception( rpc::errors::ERR_COMM );
            rpc_queue_.add_queue( slot_id );
            send_message( llu );
        }

        void call_rpc_method( const lowlevel_unit_type &llu )
        {
            if( !ready_ )
                throw vtrc::common::exception( rpc::errors::ERR_COMM );
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
            wait_result_codes qwr = rpc_queue_.read( slot_id, mess,
                                    vtrc::chrono::microseconds(microsec) );

            raise_wait_error( qwr );
        }

        void read_slot_for( uint64_t slot_id,
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

        const rpc::options *get_method_options(
                                    const gpb::MethodDescriptor *method)
        {
            upgradable_lock lck(options_map_lock_);
            static rpc::options defaults( common::defaults::method_options( ) );

            options_map_type::const_iterator f(options_map_.find(method));

            vtrc::shared_ptr<rpc::options> result;

            if( f == options_map_.end( ) ) {

                const rpc::options &serv (
                        method->service( )->options( )
                            .GetExtension( rpc::service_options ));

                const rpc::options &meth (
                        method->options( )
                            .GetExtension( rpc::method_options ));

                result = vtrc::make_shared<rpc::options>( serv );
                utilities::merge_messages( *result, defaults );
                utilities::merge_messages( *result, meth );

                upgrade_to_unique ulck( lck );
                options_map_.insert( std::make_pair( method, result ) );

            } else {
                result = f->second;
            }

            return result.get( );
        }

        void erase_all_slots(  )
        {
            rpc_queue_.erase_all( );
        }

        void cancel_all_slots( )
        {
            rpc_queue_.cancel_all( );
        }

        void cancel_all_slots( std::string )
        {
            rpc_queue_.cancel_all( );
        }

        common::rpc_service_wrapper_sptr get_service( const std::string &name )
        {
            return parent_->get_service_by_name( name );
        }

        void closure_fake( closure_holder_sptr & /*holder*/ )
        {
            //send_message( fake_ );
        }

        void closure_done( closure_holder_sptr &holder )
        {
            lowlevel_unit_type &llu = *holder->llu_;

            bool failed        = false;
            unsigned errorcode = 0;

            if( holder->controller_->IsCanceled( ) ) {

                errorcode = rpc::errors::ERR_CANCELED;
                failed = true;

             } else if( holder->controller_->Failed( ) ) {

                errorcode = rpc::errors::ERR_INTERNAL;
                failed = true;

            }

            llu.clear_request( );
            llu.clear_call( );

            if( failed ) {

                llu.mutable_error( )->set_code( errorcode );
                llu.clear_response( );

                if( !holder->controller_->ErrorText( ).empty( ) )
                    llu.mutable_error( )
                          ->set_additional( holder->controller_->ErrorText( ));

            } else {
                if( llu.opt( ).accept_response( ) ) {
                    // std::cout << "llu has response\n";
                    llu.set_response( holder->res_->SerializeAsString( ) );
                } else {
                    // std::cout << "llu has not response\n";
                }
            }
            send_message( llu );
        }

        void closure_runner( closure_holder_sptr holder, bool wait )
        {

            if( holder->proto_closure_ ) {
                delete holder->proto_closure_;
                holder->proto_closure_ = NULL;
            } else {
                return;
            }

            connection_iface_sptr lck(holder->connection_.lock( ));
            if( !lck ) return;

            if( !std::uncaught_exception( ) ) {
                closure_done( holder );
                //wait ? closure_done( holder ) : closure_fake( holder );
            } else {
//                std::cerr << "Uncaught exception at done handler for "
//                          << holder->llu_->call( ).service_id( )
//                          << "::"
//                          << holder->llu_->call( ).method_id( )
//                          << std::endl;
            }
        }

        gpb::Closure *make_closure(closure_holder_sptr &closure_hold, bool wait)
        {
            closure_hold->proto_closure_ =
                    gpb::NewPermanentCallback( this, &this_type::closure_runner,
                                               closure_hold, wait );
            return closure_hold->proto_closure_;
        }

        void make_local_call_impl( lowlevel_unit_sptr  &llu,
                                   closure_holder_sptr &closure_hold)
        {
            protocol_layer::context_holder ch( parent_, llu.get( ) );

            if( ch.ctx_->depth( ) > session_opts_.max_stack_size( ) ) {
                throw vtrc::common::exception( rpc::errors::ERR_OVERFLOW );
            }

            common::rpc_service_wrapper_sptr
                    service(get_service(llu->call( ).service_id( )));

            if( !service || !service->service( ) ) {
                throw vtrc::common::exception( rpc::errors::ERR_BAD_FILE,
                                               "Service not found");
            }

            gpb::MethodDescriptor const *meth
                (service->get_method(llu->call( ).method_id( )));

            if( !meth ) {
                throw vtrc::common::exception( rpc::errors::ERR_NO_FUNC );
            }

            const rpc::options *call_opt( parent_->get_method_options( meth ) );

            ch.ctx_->set_call_options( call_opt );

            closure_hold->req_.reset
                (service->service( )->GetRequestPrototype( meth ).New( ));

            closure_hold->res_.reset
                (service->service( )->GetResponsePrototype( meth ).New( ));

            closure_hold->req_->ParseFromString( llu->request( ) );
            closure_hold->res_->ParseFromString( llu->response( ));

            closure_hold->controller_.reset(new common::rpc_controller);

            closure_hold->connection_ = connection_->weak_from_this( );
            closure_hold->llu_        = llu;

            gpb::Closure* clos(make_closure(closure_hold, llu->opt( ).wait( )));
            ch.ctx_->set_done_closure( clos );

            precall_( *connection_, meth, *llu );

            service->service( )->CallMethod( meth,
                                     closure_hold->controller_.get( ),
                                     closure_hold->req_.get( ),
                                     closure_hold->res_.get( ), clos );
        }

        void make_local_call( protocol_layer::lowlevel_unit_sptr llu )
        {
            static const protcol_closure_type empty_call;
            make_local_call( llu, empty_call );
        }

        void make_local_call( protocol_layer::lowlevel_unit_sptr &llu,
                                      const protcol_closure_type &done )
        {
            bool failed        = true;
            unsigned errorcode = 0;
            closure_holder_sptr hold;
            try {

                hold = vtrc::make_shared<closure_holder_type>( );
                hold->internal_closure_.reset(new protcol_closure_type(done));

                make_local_call_impl( llu, hold );
                failed = false;

            } catch ( const vtrc::common::exception &ex ) {
                errorcode = ex.code( );
                llu->mutable_error( )->set_additional( ex.additional( ) );

            } catch ( const std::exception &ex ) {
                errorcode = rpc::errors::ERR_INTERNAL;
                llu->mutable_error( )->set_additional( ex.what( ) );

            } catch ( ... ) {
                errorcode = rpc::errors::ERR_UNKNOWN;
                llu->mutable_error( )->set_additional( "..." );
            }

            if( failed ) {
                llu->mutable_error( )->set_code( errorcode );
                llu->clear_response( );
            }

//            try {
                postcall_( *connection_, *llu );
//            } catch( ... ) {
//                ;;;
//            }

            if( llu->opt( ).wait( ) ) {
                if( failed ) {
                    send_message( *llu );

                    /// here we must reset internal_closure
                    /// for preventing double call
                    if( hold ) {
                        hold->internal_closure_.reset( );
                    }

                    /// call 'done';
                    if( done ) {
                        done( llu->error( ) );
                    }
                }
            }

        }

        void on_system_error(const boost::system::error_code &err,
                             const std::string &add )
        {
            set_ready( false );

            vtrc::shared_ptr<rpc::lowlevel_unit>
                            llu( new  rpc::lowlevel_unit );

            rpc::errors::container *err_cont = llu->mutable_error( );

            err_cont->set_code( err.value( ) );
            err_cont->set_category( rpc::errors::CATEGORY_SYSTEM );
            err_cont->set_fatal( true );
            err_cont->set_additional( add );

            parent_->push_rpc_message_all( llu );
        }

        void set_precall( const precall_closure_type &func )
        {
            precall_ = func ? func : empty_pre( );
        }

        void set_postcall( const postcall_closure_type &func )
        {
            postcall_ = func ? func : empty_post( );;
        }

    };

    protocol_layer::protocol_layer( transport_iface *connection, bool oddside )
        :impl_(new impl(connection, oddside, defaults::session_options( )))
    {
        impl_->parent_ = this;
    }

     protocol_layer::protocol_layer(transport_iface *connection,
                                    bool oddside,
                                    const rpc::session_options &opts)
        :impl_(new impl(connection, oddside, opts ))
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

    const rpc::options *protocol_layer::get_method_options(
                              const gpb::MethodDescriptor *method)
    {
        return impl_->get_method_options( method );
    }

    const rpc::session_options &protocol_layer::session_options( ) const
    {
        return impl_->session_opts_;
    }

    const call_context *protocol_layer::get_call_context( ) const
    {
        return impl_->get_call_context( );
    }

    void protocol_layer::configure_session( const rpc::session_options &o )
    {
        impl_->configure_session( o );
    }

    call_context *protocol_layer::top_call_context( )
    {
        return impl_->top_call_context( );
    }

    // ===============

    void protocol_layer::set_level( unsigned level )
    {
        impl_->set_level( level );
    }

    unsigned protocol_layer::get_level( ) const
    {
        return impl_->get_level( );
    }

    void protocol_layer::make_local_call(protocol_layer::lowlevel_unit_sptr llu)
    {
        impl_->make_local_call( llu );
    }

    void protocol_layer::make_local_call(protocol_layer::lowlevel_unit_sptr llu,
                                         const protcol_closure_type &done)
    {
        impl_->make_local_call( llu, done );
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

    bool protocol_layer::raw_pop( std::string &result )
    {
        return impl_->raw_pop( result );
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

    void protocol_layer::set_precall( const precall_closure_type &func )
    {
        impl_->set_precall( func );
    }

    void protocol_layer::set_postcall(const postcall_closure_type &func )
    {
        impl_->set_postcall( func );
    }


    std::string protocol_layer::get_service_name( gpb::Service *service )
    {
        return get_service_name( service->GetDescriptor( ) );
    }

    std::string protocol_layer::get_service_name(
                                            const gpb::ServiceDescriptor *sd )
    {
        return sd->full_name( );
    }

}}
