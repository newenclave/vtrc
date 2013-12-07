#include <iostream>
#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>

#include <stdlib.h>

#include "vtrc-server/vtrc-application-iface.h"
#include "vtrc-common/vtrc-thread-poll.h"
#include "vtrc-common/vtrc-sizepack-policy.h"

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

int main( )
{
    typedef vtrc::common::policies::varint_policy<unsigned> packer;
    std::string test = packer::pack( 45000 );

    std::cout << "size: " << packer::packed_length( 45000 ) << "\n";
    std::cout << "res size: " << test.size( ) << "\n";
    std::cout << packer::unpack( test.begin(), test.end() ) << "\n";

	return 0;
}
