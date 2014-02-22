#include <boost/thread.hpp>

#include <google/protobuf/message.h>

#include "vtrc-protocol-layer.h"

#include "vtrc-data-queue.h"
#include "vtrc-hasher-iface.h"
#include "vtrc-transformer-iface.h"

#include "vtrc-transport-iface.h"

namespace vtrc { namespace common {

    namespace {
        static const size_t maximum_message_length = 1024 * 1024;
    }

    struct protocol_layer::protocol_layer_impl {

        transport_iface             *connection_;
        protocol_layer              *parent_;
        hasher_iface_sptr            hasher_;
        transformer_iface_sptr       transformer_;
        data_queue::queue_base_sptr  queue_;

        boost::mutex                 write_locker_; // use strand!

        protocol_layer_impl( transport_iface *c )
            :connection_(c)
            ,hasher_(common::hasher::create_default( ))
            ,transformer_(common::transformers::none::create( ))
            ,queue_(data_queue::varint::create_parser(maximum_message_length))
        {}

        std::string prepare_data( const char *data, size_t length)
        {
            /* here is:
             *  message =
             *  <packed_size(data_length+hash_length)><hash(data)><data>
            */
            std::string result(queue_->pack_size(
                                length + hasher_->hash_size( )));

            result.append( hasher_->get_data_hash(data, length ));
            result.append( data, data + length );

            /*
             * message = transform( message )
            */

            transformer_->transform_data(
                            result.empty( ) ? NULL : &result[0],
                                            result.size( ) );
            return result;
        }

        void process_data( const char *data, size_t length )
        {
            if( length > 0 ) {
                std::string next_data(data, data + length);

                /*
                 * message = revert( message )
                */
                transformer_->revert_data( &next_data[0],
                                            next_data.size( ) );
                queue_->append( &next_data[0], next_data.size( ));
                queue_->process( );

                if( !queue_->messages( ).empty( ) ) {
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
            const size_t hash_length = hasher_->hash_size( );
            const size_t diff_len    = mess.size( ) - hash_length;
            result.ParseFromArray( mess.c_str( ) + hash_length, diff_len );
        }

        bool check_message( const std::string &mess )
        {

            boost::mutex::scoped_lock l( write_locker_ );

            const size_t hash_length = hasher_->hash_size( );
            const size_t diff_len    = mess.size( ) - hash_length;

            bool result = false;

            if( mess.size( ) >= hash_length ) {
                result = hasher_->
                         check_data_hash( mess.c_str( ) + hash_length,
                                          diff_len,
                                          mess.c_str( ) );
            }
            return result;
        }


        void set_hash_transformer( hasher_iface *new_hasher,
                                   transformer_iface *new_transformer )
        {
            boost::mutex::scoped_lock l( write_locker_ );
            if(new_hasher)      hasher_.reset(new_hasher);
            if(new_transformer) transformer_.reset(new_transformer);
        }

        void pop_message( )
        {
            queue_->messages( ).pop_front( );
        }

    };

    protocol_layer::protocol_layer( transport_iface *connection )
        :impl_(new protocol_layer_impl(connection))
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

    void protocol_layer::send_data( const char *data, size_t length )
    {
        impl_->send_data( data, length );
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

    void protocol_layer::set_hasher_transformer( hasher_iface *new_hasher,
                                        transformer_iface *new_transformer )
    {
        impl_->set_hash_transformer( new_hasher, new_transformer );
    }

    void protocol_layer::pop_message( )
    {
        impl_->pop_message( );
    }

}}
