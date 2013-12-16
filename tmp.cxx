#include <iostream>
#include <boost/shared_ptr.hpp>
#include <string>

namespace vtrc {

namespace common {

    struct hasher_iface {
        virtual ~hasher_iface( ) { }
        size_t hash_size( ) const = 0;
        virtual std::string get_data_hash(const void *data, size_t length) const = 0;
        virtual bool check_data_hash( const void *data, size_t length,
                                      const void *hash) const = 0;
    };

    struct packer_iface {
        virtual ~packer_iface( ) { }
        virtual const hasher_iface &hasher( ) = 0;
    };
}

namespace server {

    struct endpoint_iface;

    struct protocol_info_iface {
        virtual ~protocol_info_iface( ) { }
        virtual common::packer_iface &packer( ) = 0;
    };

    struct connection_info_iface {
        virtual ~connection_info_iface( ) { }
        virtual endpoint_iface &endpoint( ) = 0;
        virtual void close( ) = 0;
    };

    struct application_iface {
        virtual ~application_iface( ) { }
    };

    struct endpoint_iface {

        virtual ~endpoint_iface( ) { }
        virtual application_iface &application( ) = 0;

        virtual std::string str( ) const = 0;

        virtual void init ( ) = 0;
        virtual void start( ) = 0;
        virtual void stop ( ) = 0;
    };
}}



struct data_packer {

    std::string pack_data( const void *data, size_t length )
    {
        return std::string( static_cast<const char *>(data),
                            static_cast<const char *>(data) + length);
    }

    std::string unpack_data( const void *data, size_t length )
    {
        return std::string( static_cast<const char *>(data),
                            static_cast<const char *>(data) + length);
    }

};

struct message_packer {
    vtrc::common::hasher_iface *hasher_;
    data_packer                *packer_;

    message_packer( )
        :hasher_(vtrc::common::hasher::fake::create( ))
        ,packer_(new data_packer)
    {

    }
    ~message_packer( )
    {
        delete hasher_;
        delete packer_;
    }

    std::string pack_message( const void *data, size_t length )
    {
        std::string result(hasher_->get_data_hash(data, length));
        std::string packed( packer_->pack_data(data, length) );
        return result.append( packed.begin(), packed.end() );
    }

    std::string unpack_message( const void *data, size_t length )
    {
        std::string unpacked( packer_->unpack_data(data, length) );
        const size_t hash_size(hasher_->hash_size( ));
        if( unpacked.size( ) < hash_size ) {
            throw std::runtime_error( "bad data length" );
        }
        if( !hasher_->check_data_hash( unpacked.c_str( ) + hash_size,
                                       length - hash_size,
                                       unpacked.c_str( )) )
        {
            throw std::runtime_error( "bad data hash" );
        }
        return std::string( unpacked.begin()+hash_size, unpacked.end( ) );
    }
};



typedef vtrc::common::policies::varint_policy<size_t> size_packer_type;

class message_unpack {
    std::deque<char> data_;
public:
    template <typename IterType>
    void append_data( const IterType left, const IterType right )
    {
        data_.insert(data_.end(), left, right);
    }

    void unpack( std::list<std::string> &unpacked )
    {
        std::list<std::string> tmp;

        std::deque<char>::iterator start(data_.begin());

        const std::deque<char>::iterator end(data_.end());

        while( size_t size_length = size_packer_type::size_length( start, end )) {

            size_t result_size = size_packer_type::unpack( start, end );

            if( result_size > std::distance( start, end ) - size_length ) break;

            std::advance(start, size_length);
            std::deque<char>::iterator string_end(start);
            std::advance(string_end, result_size);
            tmp.push_back( std::string(start, string_end));
            start = string_end;
        }
        data_.erase( data_.begin(), start );
        unpacked.swap( tmp );
    }

    std::deque<char> &get_data( ) { return data_; }

};


int main( )
{
    std::string data;
    data.append(size_packer_type::pack( 10 ));
    data.append("1234567890");
    data.append(size_packer_type::pack( 20 ));
    data.append("12345678901234567890");
    data.append(size_packer_type::pack( 12 ));
    data.append("1234567890!?");

    message_unpack unpacker;
    std::list<std::string> unpack;

    unpacker.append_data(data.begin(), data.end());
    unpacker.unpack( unpack );

    std::copy(unpack.begin(), unpack.end(), std::ostream_iterator<std::string>(std::cout, "\n"));

    std::cout << "\n========\n";

    std::copy(unpacker.get_data().begin(),
              unpacker.get_data().end(),
              std::ostream_iterator<char>(std::cout, " "));

    std::cout << "\n========\n";

    char append[] = {'1'};
    unpacker.append_data( &append[0], &app[1] );
    unpacker.unpack( unpack );
    std::copy(unpack.begin(), unpack.end(), std::ostream_iterator<std::string>(std::cout, "\n"));

    std::cout << std::endl;
    return 0;
}


