#include <iostream>
#include "boost/program_options.hpp"
#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-pool-pair.h"

using namespace vtrc;

class fs_application: public server::application {
public:
    fs_application( common::pool_pair &pp )
        :server::application(pp)
    { }
};

int main( int argc, char *argv[] )
{
    common::pool_pair pp( 1, 1 );

    fs_application app( pp );

    pp.stop_all( );
    pp.join_all( );
    return 0;
}
