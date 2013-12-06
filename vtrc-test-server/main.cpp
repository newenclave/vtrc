#include <iostream>
#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>

#include <stdlib.h>

#include "vtrc-server/vtrc-application-iface.h"
#include "vtrc-common/vtrc-thread-poll.h"

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

    vtrc::common::thread_poll poll( 10 );

    size_t count = poll.size( );

    sleep( 5 );
    poll.stop( );
    std::cout << "Stop!\n";
	return 0;
}
