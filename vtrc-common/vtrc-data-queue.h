#ifndef VTRC_DATA_QUEUE_H
#define VTRC_DATA_QUEUE_H

#include <string>
#include <deque>
#include <boost/shared_ptr.hpp>

namespace vtrc { namespace common { namespace data_queue {

    typedef std::deque<std::string> message_queue_type;
    typedef std::deque<char>        plain_data_type;

    class queue_base {

        struct impl;
        impl  *impl_;

    public:

        queue_base( );
        virtual ~queue_base( );

        void append( const char *data, size_t length );

        plain_data_type             &plain_data( );
        const plain_data_type       &plain_data( ) const;

        message_queue_type          &messages( );
        const message_queue_type    &messages( ) const;

        /// TODO: fix it!
        virtual std::string pack_size( size_t size ) const = 0;
        virtual void process( ) = 0;
    };

    typedef boost::shared_ptr<queue_base> queue_base_sptr;

    namespace varint {
        queue_base *create_parser    ( size_t max_valid_length );
        queue_base *create_serializer( size_t max_valid_length );
    }

    namespace fixint {
        queue_base *create_parser    ( size_t max_valid_length );
        queue_base *create_serializer( size_t max_valid_length );
    }

}}}

#endif // VTRC_DATA_QUEUE_H
