#include <iostream>
#include <boost/asio.hpp>

#include "vtrc-server/vtrc-application-iface.h"

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

int main( )
{

    main_app mapp;

	return 0;
}
