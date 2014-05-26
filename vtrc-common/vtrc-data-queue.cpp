#include <stdexcept>

#include "vtrc-stdint.h"
#include "vtrc-data-queue.h"
#include "vtrc-sizepack-policy.h"

namespace vtrc { namespace common { namespace data_queue {

    struct queue_base::impl {

        std::deque<char>        data_;
        std::deque<std::string> packed_;
        size_t                  max_valid_length_;

        impl( size_t max_valid_length )
            :max_valid_length_(max_valid_length)
        {}

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

        size_t get_maximum_length( ) const
        {
            return max_valid_length_;
        }

        void set_maximum_length(size_t new_value)
        {
            max_valid_length_ = new_value;
        }
    };

    queue_base::queue_base( size_t max_valid_length )
        :impl_(new impl(max_valid_length))
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

    size_t queue_base::get_maximum_length( ) const
    {
        return impl_->get_maximum_length( );
    }

    void queue_base::set_maximum_length(size_t new_value)
    {
        impl_->set_maximum_length( new_value );
    }

    namespace {

        template<typename SizePackPolicy>
        class serializer_impl: public queue_base {

        public:
            serializer_impl( size_t maximim_valid )
                :queue_base(maximim_valid)
            {}

            std::string pack_size( size_t size ) const
            {
                return SizePackPolicy::pack( size );
            }

            std::string *process_one( )
            {
                process( );
                return &messages( ).back( );
            }

            void process( )
            {
                typedef SizePackPolicy SSP;

                plain_data_type &data(plain_data( ));

                if( data.size( ) > get_maximum_length( ) ) {
                    throw std::length_error( "Message is too long" );
                }

                std::string new_data( SSP::pack( data.size( ) ) );

                new_data.insert( new_data.end( ), data.begin( ), data.end( ));
                messages( ).push_back( new_data );
                data.clear( );
            }
        };

        template<typename SizePackPolicy>
        class parser_impl: public queue_base {

        public:

            parser_impl( size_t maximim_valid )
                :queue_base(maximim_valid)
            { }

            std::string pack_size( size_t size ) const
            {
                return SizePackPolicy::pack( size );
            }

            std::string *process_one( )
            {
                std::string *result = NULL;
                typedef SizePackPolicy SPP;

                plain_data_type &data(plain_data( ));

                size_t next(SPP::size_length(data.begin( ), data.end( )));

                if( next > 0 ) {

                    if( next > SPP::max_length ) {
                        throw std::length_error
                                ( "The serialized data is invalid" );
                    }

                    size_t len = SPP::unpack(data.begin( ), data.end( ));

                    if( len > get_maximum_length( ) ) {
                        //std::cout << "Message is too long " << len << "\n";
                        throw std::length_error( "Message is too long" );
                    }

                    if( (len + next) <= data.size( ) ) {

                        std::deque<char>::iterator b( data.begin( ) );
                        std::deque<char>::iterator n( b );
                        std::deque<char>::iterator e( b );

                        std::advance( n, next );
                        std::advance( e, len + next );

                        std::string mess( n, e );
                        messages( ).push_back( mess );
                        result = &messages( ).back( );
                        data.erase( b, e );
                    }
                }

                return result;
            }

            void process( )
            {
                while( process_one( ) );
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

        std::string pack_size(size_t size)
        {
            return policy_type::pack( size );
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

        std::string pack_size(size_t size)
        {
            return policy_type::pack( size );
        }

    }

}}}
