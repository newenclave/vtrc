
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
            vtrc::common::raise( vtrc::common::exception( code ) );
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

        typedef lowlevel::protocol_layer_uptr lowlevel_protocol_layer;

        typedef impl this_type;
        typedef protocol_layer parent_type;
        typedef parent_type::call_stack_type                 call_stack_type;
        typedef boost::thread_specific_ptr<call_stack_type>  call_context_ptr;

        transport_iface             *connection_;
        protocol_layer              *parent_;

        rpc_queue_type               rpc_queue_;
        vtrc::atomic<uint64_t>       rpc_index_;

        options_map_type             options_map_;
        mutable vtrc::shared_mutex   options_map_lock_;

        mutable vtrc::mutex          ready_lock_;
        bool                         ready_;
        vtrc::condition_variable     ready_var_;

        unsigned                     level_;

        rpc::session_options         session_opts_;
        lowlevel_protocol_layer      ll_processor_;

        precall_closure_type         precall_;
        postcall_closure_type        postcall_;

        impl( transport_iface *c, bool odd, const rpc::session_options &opts )
            :connection_(c)
            ,parent_(NULL)
            ,rpc_index_(odd ? 101 : 100)
            ,ready_(false)
            ,level_(0)
            ,session_opts_(opts)
            ,precall_(empty_pre( ))
            ,postcall_(empty_post( ))
        { }


        ~impl( )
        {
            //delete ll_processor_; /// temporary
        }

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

        void configure_session( const rpc::session_options &opts )
        {
            session_opts_.CopyFrom( opts );
            ll_processor_->configure( opts );
        }

        std::string prepare_data( const char *data, size_t length )
        {
            return ll_processor_->pack_message( data, length );
        }

        void process_data( const char *data, size_t length )
        {
            ll_processor_->process_data( data, length );
        }

        bool parse_and_pop( gpb::Message &result )
        {
            return ll_processor_->pop_proto_message( result );
        }

        bool raw_pop( std::string &result )
        {
            return ll_processor_->pop_raw_message( result );
        }

        size_t ready_messages_count( ) const
        {
            return ll_processor_->queue_size( );
        }

        bool message_queue_empty( ) const
        {
            return ll_processor_->queue_size( ) == 0;
        }

        bool parse_message( const std::string &mess, gpb::Message &result )
        {
            return ll_processor_->parse_raw_message( mess, result );
        }

        void set_ready( bool ready )
        {
            vtrc::unique_lock<vtrc::mutex> lck( ready_lock_ );
            ready_ = ready;
            ready_var_.notify_all( );
        }

        void set_lowlevel( lowlevel::protocol_layer_iface *ll )
        {
            ll_processor_.reset( ll );
        }

        lowlevel::protocol_layer_iface *get_lowlevel( )
        {
            return ll_processor_.get( );
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
            if( state != ready_ ) {
                ready_var_.wait( lck,
                         vtrc::bind( &this_type::state_predic, this, state ));
            }
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

        void send_message( const gpb::Message &message )
        {
            std::string ser(ll_processor_->serialize_message( message ));
            send_data( ser.c_str( ), ser.size( ) );
        }

        uint64_t next_index( )
        {
            return ( rpc_index_ += 2 );
        }

        // --------------- sett ----------------- //

        static
        void check_create_stack( )
        {
            if( NULL == context_get( ) ) {
                //std::cout << "Stack is empty!\n";
                context_reset( new call_stack_type );
            }
        }

        static
        call_context *push_call_context(vtrc::shared_ptr<call_context> cc)
        {
            check_create_stack( );
            call_context *top = top_call_context( );
            cc->set_next( top );
            context_get( )->push_front( cc );
            return context_get( )->front( ).get( );
        }

        static
        call_context *push_call_context( call_context *cc)
        {
            return push_call_context( vtrc::shared_ptr<call_context>(cc) );
        }

        static
        void pop_call_context( )
        {
            if( !context_get( )->empty( ) ) {
                context_get( )->pop_front( );
            }

            if( context_get( )->empty( ) ) {
                context_reset( );
            }
        }

        static
        void reset_call_context( )
        {
            context_reset(  );
        }

        static
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

        static
        void copy_call_stack( call_stack_type &other )
        {
            if( NULL == context_get( ) ) {
                other.clear( );
            } else {
                clone_stack( *context_get( ), other );
            }
        }

        static
        const call_context *get_call_context( )
        {
            return context_get( ) && !context_get( )->empty( )
                    ? context_get( )->front( ).get( )
                    : NULL;
        }

        static
        call_context *top_call_context( )
        {
            return (context_get( ) && !context_get( )->empty( ))
                    ? context_get( )->front( ).get( )
                    : NULL;
        }

        size_t push_rpc_message(uint64_t slot_id, lowlevel_unit_sptr mess)
        {
            return rpc_queue_.write_queue_if_exists( slot_id, mess );
        }

        size_t push_rpc_message_all( lowlevel_unit_sptr mess)
        {
            return rpc_queue_.write_all( mess );
        }

        void call_rpc_method( uint64_t slot_id, const lowlevel_unit_type &llu )
        {
            if( !ready_ ) {
                vtrc::common::raise(
                    vtrc::common::exception( rpc::errors::ERR_COMM ) );
            }
            rpc_queue_.add_queue( slot_id );
            send_message( llu );
        }

        void call_rpc_method( const lowlevel_unit_type &llu )
        {
            if( !ready_ ) {
                vtrc::common::raise(
                    vtrc::common::exception( rpc::errors::ERR_COMM ) );
            }
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

        bool slot_exists( uint64_t slot_id ) const
        {
            return rpc_queue_.queue_exists( slot_id );
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
                    //std::cout << "llu has response\n";
                    llu.set_response( holder->res_->SerializeAsString( ) );
                } else {
                    //std::cout << "llu has not response\n";
                }
            }
            //std::cout << "answer: " << llu.DebugString( ) << "\n";
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
            class  closure_impl: public gpb::Closure {

                this_type           *impl_;
                closure_holder_sptr  closure_hold_;
                bool                 wait_;

            public:

                closure_impl( this_type *im,
                              closure_holder_sptr &closure_hold, bool wait )
                    :impl_(im)
                    ,closure_hold_(closure_hold)
                    ,wait_(wait)
                { }

                void Run( )
                {
                    impl_->closure_runner(closure_hold_, wait_);
                }
            };

            closure_hold->proto_closure_ =
                    new closure_impl( this, closure_hold, wait );
//                  gpb::NewPermanentCallback( this, &this_type::closure_runner,
//                                             closure_hold, wait );
            return closure_hold->proto_closure_;
        }

        void make_local_call_impl( lowlevel_unit_sptr  &llu,
                                   closure_holder_sptr &closure_hold)
        {
            protocol_layer::context_holder ch( parent_, llu.get( ) );

            if( ch.ctx_->depth( ) > session_opts_.max_stack_size( ) ) {
                common::raise( common::exception( rpc::errors::ERR_OVERFLOW ) );
                return;
            }

            common::rpc_service_wrapper_sptr
                    service(get_service(llu->call( ).service_id( )));

            if( !service || !service->service( ) ) {
                common::raise(common::exception( rpc::errors::ERR_BAD_FILE,
                                                 "Service not found") );
                return;
            }

            gpb::MethodDescriptor const *meth
                (service->get_method(llu->call( ).method_id( )));

            if( !meth ) {
                common::raise( common::exception( rpc::errors::ERR_NO_FUNC ) );
                return;
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

    void protocol_layer::set_lowlevel( lowlevel::protocol_layer_iface *ll )
    {
        impl_->set_lowlevel( ll );
    }

    lowlevel::protocol_layer_iface *protocol_layer::get_lowlevel( )
    {
        return impl_->get_lowlevel( );
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
        return impl::push_call_context( cc );
    }

    call_context *protocol_layer::push_call_context( call_context *cc )
    {
        return impl::push_call_context( cc );
    }

    void protocol_layer::pop_call_context( )
    {
        return impl::pop_call_context( );
    }

    void protocol_layer::reset_call_stack( )
    {
        impl::reset_call_context(  );
    }

    void protocol_layer::swap_call_stack(protocol_layer::call_stack_type &other)
    {
        impl::swap_call_stack( other );
    }

    void protocol_layer::copy_call_stack(protocol_layer::call_stack_type &other)
    {
        impl::copy_call_stack( other );
    }

    const call_context *protocol_layer::get_call_context( )
    {
        return impl::get_call_context( );
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


    void protocol_layer::configure_session( const rpc::session_options &o )
    {
        impl_->configure_session( o );
    }

    call_context *protocol_layer::top_call_context( )
    {
        return impl::top_call_context( );
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

    size_t protocol_layer::ready_messages_count( ) const
    {
        return impl_->ready_messages_count( );
    }

    bool protocol_layer::message_queue_empty( ) const
    {
        return impl_->message_queue_empty( );
    }

    bool protocol_layer::parse_and_pop( gpb::Message &result )
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

    bool protocol_layer::slot_exists( uint64_t slot_id ) const
    {
        return impl_->slot_exists( slot_id );
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

    size_t protocol_layer::push_rpc_message( uint64_t slot_id,
                                             lowlevel_unit_sptr mess)
    {
        return impl_->push_rpc_message(slot_id, mess);
    }

    size_t protocol_layer::push_rpc_message_all( lowlevel_unit_sptr mess)
    {
        return impl_->push_rpc_message_all( mess );
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
