#include <stdexcept>
#include <stdint.h>

#include "vtrc-data-queue.h"
#include "vtrc-sizepack-policy.h"

namespace vtrc { namespace common { namespace data_queue {

    struct queue_base::queue_base_impl {

        std::deque<char>        data_;
        std::deque<std::string> packed_;

        void append( const char *data, size_t length )
        {
            data_.insert( data_.end( ), data, data + length );
        }

        size_t data_size( ) const
        {
            return data_.size( );
        }

        size_t size( ) const
        {
            return packed_.size( );
        }

        void grab_all( std::deque<std::string> &messages )
        {
            std::deque<std::string> tmp;
            packed_.swap( tmp );
            messages.swap( tmp );
        }

        std::string &front( )
        {
            packed_.front( );
        }

        void pop_front( )
        {
            packed_.pop_front( );
        }

        plain_data_type &plain_data( )
        {
            return data_;
        }

        const plain_data_type &plain_data( ) const
        {
            return data_;
        }

        message_queue_type &messages( )
        {
            return packed_;
        }

        const message_queue_type &messages( ) const
        {
            return packed_;
        }
    };

    queue_base::queue_base( )
        :impl_(new queue_base_impl)
    {}

    queue_base::~queue_base( )
    {
        delete impl_;
    }

    void queue_base::append( const char *data, size_t length )
    {
        impl_->append( data, length );
    }

    plain_data_type &queue_base::plain_data( )
    {
        return impl_->plain_data( );
    }

    const plain_data_type &queue_base::plain_data( ) const
    {
        return impl_->plain_data( );
    }

    message_queue_type &queue_base::messages( )
    {
        return impl_->messages( );
    }

    const message_queue_type &queue_base::messages( ) const
    {
        return impl_->messages( );
    }

    namespace {

        template<typename SizePackPolicy>
        class serializer_impl: public queue_base {
            size_t maximim_valid_;
        public:
            serializer_impl( size_t maximim_valid )
                :maximim_valid_(maximim_valid)
            {}

            void process( )
            {
                typedef SizePackPolicy SSP;

                plain_data_type &data(plain_data( ));

                if( data.size( ) > maximim_valid_ )
                    throw std::length_error( "Message is too long" );

                std::string new_data( SSP::pack( data.size( ) ) );

                new_data.insert( new_data.end( ), data.begin( ), data.end( ));
                messages( ).push_back( new_data );
                data.clear( );
            }
        };

        template<typename SizePackPolicy>
        class parser_impl: public queue_base {

            size_t maximim_valid_;

        public:
            parser_impl( size_t maximim_valid )
                :maximim_valid_(maximim_valid)
            {}

            void process( )
            {
                typedef SizePackPolicy SPP;
                plain_data_type &data(plain_data( ));

                size_t next;
                bool valid_data = true;
                while( valid_data &&
                     ( next = SPP::size_length(data.begin( ), data.end( ))) )
                {

                    if( next > SPP::max_length ) {
                        throw std::length_error
                                ( "The serialized data is invalid" );
                    }

                    size_t len = SPP::unpack(data.begin( ), data.end( ));

                    if( len > maximim_valid_ )
                        throw std::length_error( "Message is too long" );

                    if( (len + next) <= data.size( ) ) {

                        std::deque<char>::iterator b( data.begin( ) );
                        std::deque<char>::iterator n( b );
                        std::deque<char>::iterator e( b );

                        std::advance( n, next );
                        std::advance( e, len + next );

                        std::string mess( n, e );
                        messages( ).push_back( mess );
                        data.erase( b, e );

                    } else {
                        valid_data = false;
                    }
                }

            }
        };
    }

    namespace varint {
        typedef vtrc::common::policies::varint_policy<size_t> policy_type;
        queue_base *create_parser( size_t max_valid_length )
        {
            return new parser_impl<policy_type>(max_valid_length);
        }
        queue_base *create_serializer( size_t max_valid_length )
        {
            return new serializer_impl<policy_type>(max_valid_length);
        }
    }

    namespace fixint {
        typedef vtrc::common::policies::fixint_policy<uint32_t> policy_type;
        queue_base *create_parser( size_t max_valid_length )
        {
            return new parser_impl<policy_type>(max_valid_length);
        }
        queue_base *create_serializer( size_t max_valid_length )
        {
            return new serializer_impl<policy_type>(max_valid_length);
        }
    }

    namespace fixint64 {
        typedef vtrc::common::policies::fixint_policy<uint64_t> policy_type;
        queue_base *create_parser( size_t max_valid_length )
        {
            return new parser_impl<policy_type>(max_valid_length);
        }
        queue_base *create_serializer( size_t max_valid_length )
        {
            return new serializer_impl<policy_type>(max_valid_length);
        }
    }

}}}
