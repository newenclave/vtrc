#include <iostream>
#include <fcntl.h>
#include <vector>

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace ba = boost::asio;
std::vector<char> data(4096);

void start_read( ba::posix::stream_descriptor &desc );

void read_handler( boost::system::error_code const &e,
                   size_t bytes,
                   ba::posix::stream_descriptor &desc)
{
    if( !e ) {
        std::cout << "read " << bytes << "\n";
        start_read( desc );
    } else {
        throw std::runtime_error( e.message( ) );
    }
}

void start_read( ba::posix::stream_descriptor &desc )
{
    desc.async_read_some( ba::buffer(data),
                          boost::bind( &read_handler,
                                       ba::placeholders::error,
                                       ba::placeholders::bytes_transferred,
                                       boost::ref(desc)) );
}

int main( int argc, const char *argv[] ) try
{

    ba::io_service ios;
    ba::io_service::work w(ios);

    ba::posix::stream_descriptor sd(ios);

    int fd = open( argv[1], O_RDONLY );

    std::cout << "Fd: " << fd << "\n";

    sd.assign( fd );
    start_read( sd );

    while( 1 ) {
        ios.run_one( );
    }

    return 0;
} catch( const std::exception &ex ) {
    std::cerr << ex.what( ) << "\n";
    return 1;
}

