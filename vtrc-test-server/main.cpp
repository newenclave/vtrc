#include <iostream>
#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>

#include <stdlib.h>

#include "vtrc-server/vtrc-application-iface.h"
#include "vtrc-common/vtrc-thread-poll.h"
#include "vtrc-common/vtrc-sizepack-policy.h"
#include "vtrc-common/vtrc-hasher-iface.h"
#include "vtrc-common/vtrc-hasher-impls.h"

namespace ba = boost::asio;

class main_app: public vtrc::server::application_iface
{
    ba::io_service ios_;
public:
    main_app( )
    {}
    ba::io_service &get_io_service( )
    {
        return ios_;
    }
};

void print( )
{

    std::cout << "hello\n";
}

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
            throw std::length_error( "bad data length" );
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

int main( )
{
    typedef vtrc::common::policies::varint_policy<unsigned> packer;
    std::string test = packer::pack( 45000 );

    std::cout << "size: " << packer::packed_length( 45000 ) << "\n";
    std::cout << "res size: " << test.size( ) << "\n";
    std::cout << packer::unpack( test.begin(), test.end() ) << "\n";

	return 0;
}
