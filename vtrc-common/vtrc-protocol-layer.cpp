#include <boost/thread.hpp>
#include <boost/thread/tss.hpp>

#include <boost/atomic.hpp>

#include <google/protobuf/message.h>

#include "vtrc-protocol-layer.h"

#include "vtrc-data-queue.h"
#include "vtrc-hasher-iface.h"
#include "vtrc-transformer-iface.h"

#include "vtrc-transport-iface.h"

#include "vtrc-conditional-queues.h"
#include "vtrc-exception.h"
#include "vtrc-call-context.h"

#include "protocol/vtrc-rpc-lowlevel.pb.h"
#include "protocol/vtrc-errors.pb.h"

namespace vtrc { namespace common {

    namespace gpb = google::protobuf;

    namespace {

        void raise_error( unsigned code )
        {
            throw vtrc::common::exception( code );
        }

        void raise_wait_error( queue_wait_result wr )
        {
            switch ( wr ) {
            case QUEUE_WAIT_CANCELED:
                raise_error( vtrc_errors::ERR_CANCELED );
                break;
            case QUEUE_WAIT_TIMEOUT:
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
        typedef boost::shared_ptr<ll_unit_type>  ll_unit_sptr;

        typedef conditional_queues<rpc_unit_index, ll_unit_sptr> rpc_queue_type;

        typedef boost::thread_specific_ptr<call_context> call_context_ptr;
    }

    struct protocol_layer::impl {

        transport_iface             *connection_;
        protocol_layer              *parent_;
        hasher_iface_sptr            sign_maker_;
        hasher_iface_sptr            sign_checker_;
        transformer_iface_sptr       transformer_;
        transformer_iface_sptr       reverter_;

        data_queue::queue_base_sptr  queue_;

        rpc_queue_type               rpc_queue_;
        boost::atomic_int64_t        rpc_index_;

        call_context_ptr             context_;

        impl( transport_iface *c )
            :connection_(c)
            ,sign_maker_(common::hasher::create_default( ))
            ,sign_checker_(common::hasher::create_default( ))
            //,transformer_(common::transformers::erseefor::create( "1234", 4 ))
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
                                length + sign_maker_->hash_size( )));

            result.append( sign_maker_->get_data_hash(data, length ));
            result.append( data, data + length );

            /*
             * message = transform( message )
            */

            transformer_->transform(
                            result.empty( ) ? NULL : &result[0],
                                            result.size( ) );
            return result;
        }

        void process_data( const char *data, size_t length )
        {
            if( length > 0 ) {
                std::string next_data(data, data + length);

                size_t old_size = queue_->messages( ).size( );

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

        data_queue::queue_base &get_data_queue( )
        {
            return *queue_;
        }

        const data_queue::queue_base &get_data_queue( ) const
        {
            return *queue_;
        }

        void parse_message( const std::string &mess,
                            google::protobuf::Message &result )
        {
            const size_t hash_length = sign_checker_->hash_size( );
            const size_t diff_len    = mess.size( ) - hash_length;
            result.ParseFromArray( mess.c_str( ) + hash_length, diff_len );
        }

        bool check_message( const std::string &mess )
        {
            const size_t hash_length = sign_checker_->hash_size( );
            const size_t diff_len    = mess.size( ) - hash_length;

            bool result = false;

            if( mess.size( ) >= hash_length ) {
                result = sign_checker_->
                         check_data_hash( mess.c_str( ) + hash_length,
                                          diff_len,
                                          mess.c_str( ) );
            }
            return result;
        }

        void clear_call_context( )
        {
            context_.reset( );
        }

        call_context *create_call_context( vtrc_rpc_lowlevel::lowlevel_unit *ll)
        {
            context_.reset( new call_context( ll ) );
            return context_.get( );
        }

        const call_context *get_call_context( ) const
        {
            return context_.get( );
        }

        void change_sign_checker( hasher_iface *new_signer )
        {
            sign_checker_.reset(new_signer);
        }

        void change_sign_maker( hasher_iface *new_signer )
        {
            sign_maker_.reset(new_signer);
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

        void push_rpc_message(uint64_t slot_id,
                    boost::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit> mess)
        {
            if( rpc_queue_.queue_exists( slot_id ) )
                rpc_queue_.write_queue( slot_id, mess );
        }

        void call_rpc_method( uint64_t slot_id,
                              const vtrc_rpc_lowlevel::lowlevel_unit &llu )
        {
            rpc_queue_.add_queue( slot_id );
            send_message( llu );
        }

        void call_rpc_method( const vtrc_rpc_lowlevel::lowlevel_unit &llu )
        {
            send_message( llu );
        }

        void wait_call_slot( uint64_t slot_id, uint32_t millisec)
        {
            queue_wait_result qwr = rpc_queue_.wait_queue( slot_id,
                             boost::chrono::milliseconds(millisec) );
            raise_wait_error( qwr );
        }

        void wait_call_slot(
                    uint64_t slot_id,
                    std::deque<
                          boost::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit>
                    > &data_list,
                    uint32_t millisec )
        {
            queue_wait_result qwr = rpc_queue_.read_queue(
                        slot_id, data_list,
                        boost::chrono::milliseconds(millisec)) ;
            raise_wait_error( qwr );
        }

        void close_slot( uint64_t slot_id )
        {
            rpc_queue_.erase_queue( slot_id );
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

    call_context *protocol_layer::create_call_context(
                            vtrc_rpc_lowlevel::lowlevel_unit *llu )
    {
        return impl_->create_call_context( llu );
    }

    void protocol_layer::clear_call_context( )
    {
        impl_->clear_call_context( );
    }

    const call_context *protocol_layer::get_call_context( ) const
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

    data_queue::queue_base &protocol_layer::get_data_queue( )
    {
        return impl_->get_data_queue( );
    }

    const data_queue::queue_base &protocol_layer::get_data_queue( ) const
    {
        return impl_->get_data_queue( );
    }

    void protocol_layer::change_sign_maker( hasher_iface *new_hasher )
    {
        impl_->change_sign_maker( new_hasher );
    }

    void protocol_layer::change_sign_checker( hasher_iface *new_hasher )
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

    void protocol_layer::call_rpc_method( const
                                        vtrc_rpc_lowlevel::lowlevel_unit &llu )
    {
        impl_->call_rpc_method( llu );
    }

    void protocol_layer::call_rpc_method( uint64_t slot_id,
                          const vtrc_rpc_lowlevel::lowlevel_unit &llu )
    {
        impl_->call_rpc_method( slot_id, llu );
    }

    void protocol_layer::wait_call_slot( uint64_t slot_id, uint32_t millisec)
    {
        impl_->wait_call_slot( slot_id, millisec);
    }

    void protocol_layer::wait_call_slot( uint64_t slot_id,
                         std::deque<
                            boost::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit>
                         > &data_list,
                         uint32_t millisec )
    {
        impl_->wait_call_slot( slot_id, data_list, millisec);
    }

    void protocol_layer::close_slot(uint64_t slot_id)
    {
        impl_->close_slot( slot_id );
    }

    void protocol_layer::push_rpc_message(uint64_t slot_id,
                      boost::shared_ptr<vtrc_rpc_lowlevel::lowlevel_unit> mess)
    {
        impl_->push_rpc_message(slot_id, mess);
    }

}}
